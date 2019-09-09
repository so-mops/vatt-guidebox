#include <stdio.h>   /* Standard input/output definitions */
#include <string.h>  /* String function definitions */
#include <unistd.h>  /* UNIX standard function definitions */
#include <fcntl.h>   /* File control definitions */
#include <errno.h>   /* Error number definitions */
#include <termios.h> /* POSIX terminal control definitions */
#include <stdlib.h>
#include <sys/select.h>
#include <sys/types.h>

#include "gb_serial.h"

#define error_message printf

const char * STATUS_CODES[4][16] = {
	{
		"Drive Ready",
		"Bo: Motor is off (indicator)",
		"Bt: Trajectory in progress (indicator)",
		"Servo Bus Voltage Fault",
		"Peak Over Current occurred",
		"Excessive Temperature fault latch",
		"Excessive Position Error Fault",
		"Velocity Limit Fault",
		"Real-time temperature limit",
		"Derivative Error Limit (dE/dt) Fault",
		"Hardware Limit Positive Enabled",
		"Hardware Limit Negative Enabled",
		"Historical Right Limit (+ or Positive)",
		"Historical Left Limit (- or Negative)",
		"Right ( + or Positive) Limit Asserted",
		"Left Limit ( - or Negative) Asserted",
	},
	{
		"Rise Capture Encoder(0) Armed",
		"Fall Capture Encoder(0) Armed",
		"Rising edge captured ENC(0) (historical bit)",
		"Falling edge captured ENC(0) (historical bit)",
		"Rise Capture Encoder(1) Armed",
		"Fall Capture Encoder(1) Armed",
		"Rising edge captured ENC(1) (historical bit)",
		"Falling edge captured ENC(1) (historical bit)",
		"Capture input state 0 (indicator)",
		"Capture input state 1 (indicator)",
		"Software Travel Limits Enabled",
		"Soft limit mode (indicator): 0-Don’t Stop. 1-Cause Fault. Default is 1",
		"Historical positive software over travel limit",
		"Historical negative software over travel limit",
		"Real time positive soft limit (indicator)",
		"Real time negative soft limit (indicator)",
	},

	{
		"Error on Communications Channel 0",
		"Error on Communications Channel 1",
		"USB Error (Class 6 only)",
		"Reserved 3",
		"CAN Port Error",
		"Reserved 5",
		"Ethernet Error (Class 6 only)",
		"I2C Running",
		"Watchdog Event",
		"ADB (Animatics Data Block) Bad Checksum",
		"Program Running",
		"Trace in Progress",
		"EE Write Buffer Overflow",
		"EE Busy",
		"Command Syntax Error",
		"Program Checksum Error",
	},

	{
		"Reserved 0",
		"Torque Saturation",
		"Voltage Saturation",
		"Wraparound Occurred",
		"KG Enabled",
		"Velocity Direction",
		"Torque Direction",
		"I/O Fault Latch",
		"Relative Position Mode",
		"Reserved 9",
		"Peak Current Saturation",
		"Modulo Rollover",
		"Brake Asserted",
		"Brake OK",
		"External Go Enabled",
		"Velocity Target Reached",
	}

};

int
set_interface_attribs (int fd, int speed, int parity)
{
        struct termios tty;
        memset (&tty, 0, sizeof tty);
        if (tcgetattr (fd, &tty) != 0)
        {
                error_message ("error %d from tcgetattr", errno);
                return -1;
        }

        cfsetospeed (&tty, speed);
        cfsetispeed (&tty, speed);

        tty.c_cflag = (tty.c_cflag & ~CSIZE) | CS8;     // 8-bit chars
        // disable IGNBRK for mismatched speed tests; otherwise receive break
        // as \000 chars
        tty.c_iflag &= ~IGNBRK;         // disable break processing
        tty.c_lflag = 0;                // no signaling chars, no echo,
                                        // no canonical processing
        tty.c_oflag = 0;                // no remapping, no delays
        tty.c_cc[VMIN]  = 0;            // read doesn't block
        tty.c_cc[VTIME] = 5;            // 0.5 seconds read timeout

        tty.c_iflag &= ~(IXON | IXOFF | IXANY); // shut off xon/xoff ctrl

        tty.c_cflag |= (CLOCAL | CREAD);// ignore modem controls,
                                        // enable reading
        tty.c_cflag &= ~(PARENB | PARODD);      // shut off parity
        tty.c_cflag |= parity;
        tty.c_cflag &= ~CSTOPB;
        tty.c_cflag &= ~CRTSCTS;

        if (tcsetattr (fd, TCSANOW, &tty) != 0)
        {
                error_message ("error %d from tcsetattr", errno);
                return -1;
        }
        return 0;
}

void
set_blocking (int fd, int should_block)
{
        struct termios tty;
        memset (&tty, 0, sizeof tty);
        if (tcgetattr (fd, &tty) != 0)
        {
                error_message ("error %d from tggetattr", errno);
                return;
        }

        tty.c_cc[VMIN]  = should_block ? 1 : 0;
        tty.c_cc[VTIME] = 5;            // 0.5 seconds read timeout

        if (tcsetattr (fd, TCSANOW, &tty) != 0)
                error_message ("error %d setting term attributes", errno);
}



/*
 * 'open_port()' - Open serial port 1.
 *
 * Returns the file descriptor on success or -1 on error.
 */

int open_port( char usbport[] )
{
	int fd; /* File descriptor for the port */

	char serialfix = 128;


	/*fd = open(usbport, O_RDWR | O_NOCTTY | O_NDELAY);
	fcntl(fd, F_SETFL, FNDELAY);
	if (fd == -1)
	{
		//Could not open the port.
		perror("open_port: Unable to open port");
	}


	else
		fcntl(fd, F_SETFL, 0);*/


	fd = open (usbport, O_RDWR | O_NOCTTY | O_SYNC);
	if (fd < 0)
		{
        	error_message ("error %d opening %s: %s", errno, usbport, strerror (errno));
        	return (fd);
		}	

	set_interface_attribs (fd, B9600, 0);  // set speed to 9600 bps, 8n1 (no parity)
	set_blocking (fd, 0);                // set no blocking
	
  // this is necessary, not sure why.
	write (fd, &serialfix, 1);           // send 1 character greeting

	
	
	return (fd);
}

/*
 * 'close_port()' - close serial port 1.
 *
 * 
 */

int close_port( int fd )
{
	close(fd);
}


int moog_write( int rs485_fd, const char *msg )
{
	char send_buffer[SENDSIZE];
	if( strlen(msg) > SENDSIZE-1 ) 
		return -1; //Buffer oversize.

	
	snprintf(send_buffer, strlen(msg)+2, "%s ", msg );
	printf("sending <%s>\n", send_buffer);
	write( rs485_fd, send_buffer, strlen(send_buffer) );
}


int moog_read( int rs485_fd, char resp[] )
{
	int rn=1;
	int ii=0;
	fd_set set;
	struct timeval timeout;
	timeout.tv_sec = 0;
	timeout.tv_usec = 500000;
	FD_ZERO(&set);
	FD_SET(rs485_fd, &set);
	

	while(rn > 0)
	{
		if(select( rs485_fd+1, &set, NULL, NULL, &timeout ) == 1)
		{
			rn = read(rs485_fd, resp+ii, 1);
			printf("%i %c\n", ii, resp[ii]);
			if(resp[ii] == '\r')
			{
				resp[ii] = '\0';
				return 0;// finished line
			}
			ii++;
		}
		else
		{
			break;
		}
	}
	return -1;// there is no data on the line
}



int moog_callsub( int rs485_fd, int subnum, int can_addr )
{

	char msg[12];
	if( can_addr == -1 )//head node
		snprintf( msg, 12, "GOSUB(%i)", subnum );
	else
		snprintf( msg, 20, "GOSUB(%i):%i", subnum, can_addr );
	moog_write( rs485_fd, msg );
	
	return 0;
}



/****************************************************
 * Name: moog_init
 * Args: rs485_fd -> file descript of serial port
 * Description: 
 * 		Sends a reset, 'Z', and calls the head node
 * 		subroutine on the head node. This starts 
 * 		the head node and then triggers all
 * 		nodes to ready themselves.
 *
 *
 * 		-Scott Swindell 8/2019
 *********************************************/
int moog_init(int rs485_fd )
{
	
	char resp[10000];

	moog_write(rs485_fd, "Z:0"); //reset all nodes
	sleep(1);
	moog_write(rs485_fd, "GOSUB(0)" ); // start head node
	usleep(200000);

	while ( moog_read(rs485_fd, resp) !=-1 )
	{
		printf("%s\n", resp);// starting head node print a bunch of stuff
	}
	
	return 0;
}

/****************************************************
 * Name: moog_home
 * args: rs485_fd-> serial file descriptor
 * 		can_addr-> motor number
 *
 *
 * Description:
 * 		Send a home command to a motor. For the 
 * 		linear stages this is the subroutine
 * 		100 for the filter wheels it is 
 * 		101. 
 *
 *
 * **************************************************/
int moog_home( int rs485_fd, int can_addr )
{
	switch(can_addr)
	{
		case OFFSET_X:
		case OFFSET_Y:
		case OFFSET_FOCUS:
			moog_callsub( rs485_fd, 100, can_addr );
		break;
		default:
			printf("Motor %i can not be homed yet", can_addr);
			return -1;

	}
	return 0;
}


/****************************************************
 * Name: moog_lgoto
 * args: rs485_fd-> serial file descriptor
 *      can_addr-> motor number
 *      pos-> integer position to send to the motor
 *
 *
 * Description:
 *       Sends one of the linear stage motors
 *       to a new position. This works by setting 
 *       the position target (PT) on the motor
 *       and then sending the go command (G)
 *       
 *
 * **************************************************/
int moog_lgoto(int rs485_fd, int can_addr, int pos )
{
	char msg[40];
	snprintf( msg, 40, "PT:%i=%i", can_addr, pos );
	moog_write(rs485_fd, (const char *) msg);
	usleep(100); // this is probably not necessary
	moog_write(rs485_fd, "G"); //GO!

	return 0;
}



/****************************************************
 * Name: moog_fgoto
 * args: rs485_fd-> serial file descriptor
 *      can_addr-> motor number
 *      fnum-> integer filter number. 
 *
 *
 * Description:
 * 		Moves one of the given filer wheel to the 
 * 		fitler number. This is done by 
 * 		setting the f variable on the motor using
 * 		f:can_addr=fnum and movign the motor 
 * 		by calling the subroutine gosub(400).
 *
 *
 * **************************************************/
int moog_fgoto( int rs485_fd, int can_addr, int fnum )
{
	char msg[10];
    snprintf( msg, 10, "f:%i=%i", can_addr, fnum );
	moog_write(rs485_fd, (const char *) msg);
	moog_callsub(rs485_fd, 400, can_addr);//call the subroutine to move fwheel
	usleep(100); // this is probably not necessary
	//moog_callsub(rs485_fd, )
}

/*******************************
 * Name: moog_getstatus
 * args: rs485_fd-> serial file descriptor
 * 		stat ->  status struct to be populated
 *
 * Description:
 * 		Populates the given MSTATUS structure
 * 		by querying the drive for various
 * 		status words with the RW(WORDNUM):motor_num
 * 		command. Each status word is a 16 bit number
 * 		and the status codes are contained in the 
 * 		STATUS_CODES array. 
 *
 * 		TODO: add other important statuses to the
 * 		struct like a motion bit and current speed
 * 		and acceleration. 
 *
 *		
 *
 * ***************************/
int moog_getstatus(int rs485_fd, MSTATUS* stat)
{
	char msg[10];
	char resp[READSIZE];
	int error;
	for(int ii=0; ii<4; ii++)
	{

		snprintf( msg, 10, "RW(%i):%i", ii, stat->motor_num );
		moog_write( rs485_fd, (const char *) msg );
		error = moog_read(rs485_fd, resp);
		stat->words[ii] = atoi(resp);

	}
	
}


/**********************************
 * Name: print_status
 * Arg: stat -> status struct to be printed
 *
 * Description:
 * 		Pretty prints the status struct
 * 		using the STATUS_CODES array.
 *
 * ********************************/
void print_status(MSTATUS stat)
{
	printf("Motor %i %s:\n", stat.motor_num, stat.name);

	//report the first 4 status words
	for( int word_ii=0; word_ii<4; word_ii++ )
	{
		printf("  Word%i == %i\n", word_ii,  stat.words[word_ii]);
		for(int bit=0; bit<16; bit++)
		{
			if( (1<<bit) & stat.words[word_ii] )
			{//Print the error code descript of each status word bit
				printf("    %s\n", STATUS_CODES[word_ii][bit]);
			}
		}
	}
	printf("\n\n");
}


/*******************************
 * Name: build_stat_structs
 * args: rs485_fd-> serial file descriptor
 * 		motors - > arrary of status structs to be built
 *
 * Description:
 * 		Uses the gosub(999) subroutine call
 * 		to get a list of all the motor names 
 * 		and their numbers from from the serial 
 * 		line. This information is then stored 
 * 		in the array of status structs. 
 *
 *
 *
 * ***************************/
int build_stat_structs( int rs485_fd, MSTATUS motors[] )
{
	char resp[READSIZE];
	moog_read(rs485_fd, resp); //flush line
	int wc;
	moog_callsub(rs485_fd, 999, -1);

	moog_read( rs485_fd, resp);
	wc = sscanf(resp, "MOTOR #%i %s", &motors[0].motor_num, motors[0].name );
	

	moog_read( rs485_fd, resp);
	wc = sscanf(resp, "MOTOR #%i %s", &motors[1].motor_num, motors[1].name );

	moog_read( rs485_fd, resp);
	wc = sscanf(resp, "MOTOR #%i %s", &motors[2].motor_num, motors[2].name );

	moog_read( rs485_fd, resp);
	wc = sscanf(resp, "MOTOR #%i %s", &motors[3].motor_num, motors[3].name );
	
	moog_read( rs485_fd, resp);
	wc = sscanf(resp, "MOTOR #%i %s", &motors[4].motor_num, motors[4].name );

	moog_read( rs485_fd, resp);
	wc = sscanf(resp, "MOTOR #%i %s", &motors[5].motor_num, motors[5].name );

	moog_read( rs485_fd, resp);
	wc = sscanf(resp, "MOTOR #%i %s", &motors[6].motor_num, motors[6].name );
}


MSTATUS * motor_by_name(char name[], MSTATUS motors[] )
{
	MSTATUS * motor;
	for(motor=motors; motor<motors+7; motors++)
		if(strcmp( motor->name, name ) == 0)
			return motor;
}




