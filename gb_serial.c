#include <stdio.h>   /* Standard input/output definitions */
#include <string.h>  /* String function definitions */
#include <unistd.h>  /* UNIX standard function definitions */
#include <fcntl.h>   /* File control definitions */
#include <errno.h>   /* Error number definitions */
#include <termios.h> /* POSIX terminal control definitions */
#include <stdlib.h>
#include <sys/select.h>
#include <sys/types.h>
#include <netdb.h> 
#include <stdio.h> 
#include <stdlib.h> 
#include <string.h> 
#include <sys/socket.h> 
#include <netinet/in.h>
#include <arpa/inet.h>

#define MAX 80 
#define PORT 10001
#define LANTRONIX "10.130.133.24" 
#define SA struct sockaddr 

#include "gb_serial.h"

#define error_message printf
#define NMOTORS 7
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

/*############################################################################
#  Title: lantronix_reset()
#  Author: C.Johnson
#  Date: 10/2/19
#  Args: char *host -> address of lantronix
	int port -> port that telnet server runs on(usually 9999 on lantronix)
#  Returns: N/A
#  Description: Resets the Lantronix UDS1100 RS485/Network adapter.  This
#	is done in a hacky way by talking to the telnet server with
#	sleeps to delay between communications.
#
#############################################################################*/

void gb_lantronix_reset(char *host, short port)
{
int sockfd, connfd, x;
char serialfix=128; 
char buff[500];
struct sockaddr_in servaddr, cli; 
  
	// socket create and varification 
	sockfd = socket(AF_INET, SOCK_STREAM, 0); 
	if (sockfd == -1) 
		{ 
		printf("socket creation failed...\n"); 
		exit(0); 
		} 
	else
	
	printf("Socket successfully created..\n"); 
	bzero(&servaddr, sizeof(servaddr)); 
  
	// assign IP, PORT 
	servaddr.sin_family = AF_INET; 
	servaddr.sin_addr.s_addr = inet_addr(host); 
	servaddr.sin_port = htons(port); 
  
	// connect the client socket to server socket 
	if (connect(sockfd, (struct sockaddr*)&servaddr, sizeof(servaddr)) != 0) 
		{ 
		printf("connection with the server failed...\n"); 
		exit(0); 
		} 
	else
		printf("connected to the server..\n"); 

	

	//first wait for the connection message
	sleep(2);
	memset(buff, 0, sizeof(buff));
	x=recv(sockfd, buff, 500, MSG_DONTWAIT);
	//printf("x=%i chars\n", x);
	//printf("buff=%s\n", buff);

	//send a carriage return to enter the menu
	//then wait for the menu items
	write(sockfd, "\r", 1);
	sleep(2);
	memset(buff, 0, sizeof(buff));
	x=recv(sockfd, buff, sizeof(buff), MSG_DONTWAIT);
	//printf("x=%i chars\n", x);
	//printf("buff=%s\n", buff);

	//select option 9 "save and exit", and 
	//send another carriage return
	write(sockfd, "9\r", 2);
	close(sockfd);

	sleep(5);


}

/*############################################################################
#  Title: open_port_net()
#  Author: C.Johnson
#  Date: 9/9/19
#  Args: 
#  Returns: 
#
#############################################################################*/
int open_port_net(char *host, short port) 
{ 
int sockfd, connfd;
char serialfix=128; 
struct sockaddr_in servaddr, cli; 
  
	// socket create and varification 
	sockfd = socket(AF_INET, SOCK_STREAM, 0); 
	if (sockfd == -1) 
		{ 
		printf("socket creation failed...\n"); 
		exit(0); 
		} 
	else
	
	printf("Socket successfully created..\n"); 
	bzero(&servaddr, sizeof(servaddr)); 
  
	// assign IP, PORT 
	servaddr.sin_family = AF_INET; 
	servaddr.sin_addr.s_addr = inet_addr(host); 
	servaddr.sin_port = htons(port); 
  
	// connect the client socket to server socket 
	if (connect(sockfd, (SA*)&servaddr, sizeof(servaddr)) != 0) 
		{ 
		printf("connection with the server failed...\n"); 
		exit(0); 
		} 
	else
		printf("connected to the server..\n"); 

	int status = fcntl(sockfd, F_SETFL, fcntl(sockfd, F_GETFL, 0) | O_NONBLOCK);

	if (status == -1)
		{
		perror("calling fcntl");
		// handle the error.  By the way, I've never seen fcntl fail in this way
		}

	moog_write(sockfd, &serialfix);//
	

	return sockfd;
  
} 

/*############################################################################
#  Title: set_interface_attribs
#  Author: C.Johnson
#  Date: 9/9/19
#  Args:  int fd -> tty port file descriptor
#	  int speed -> serial speed
#	  int polarity -> serial polarity
#  Returns: 0 on success, non zero on failure
#  Description: setup tty port serial attibutes for port referred to  by fd
#
#############################################################################*/
int set_interface_attribs (int fd, int speed, int parity)
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

/*############################################################################
#  Title: set_blocking
#  Author: C.Johnson
#  Date: 9/9/19
#  Args:  int fd -> tty port file descriptor
#	  int should_block -> 1 for blocking, 0 for non blocking
#  Returns: N/A
#  Description: setup tty port blocking attribute for port referred to  by fd
#
#############################################################################*/
void set_blocking (int fd, int should_block)
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

	
	fd = open (usbport, O_RDWR | O_NOCTTY | O_SYNC);
	if (fd < 0)
		{
   		error_message ("error %d opening %s: %s", errno, usbport, strerror (errno));
      	return (fd);
		}	

	//set_interface_attribs (fd, B9600, 0);  // set speed to 9600 bps, 8n1 (no parity)
	set_interface_attribs (fd, B115200, 0);  // set speed to 115200 bps, 8n1 (no parity)
	set_blocking (fd, 0);                // set no blocking
	
	// this is necessary, not sure why.
	moog_write(fd, &serialfix);
	
	
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
	//fprintf(stderr, "sending <%s>\n", send_buffer);
	write( rs485_fd, send_buffer, strlen(send_buffer) );
}

/********************************************************
 * Name: moog_read
 * args: rs485_fd-> filedescriptor for serial port
 *       resp -> pointer to char array to be filled
 *       with data from the serial port
 * Descr: Read one line of data from (ending in \n or \r)
 *          and put that data in the resp char array.
 *          The serial line is non blocking 
 *          this function should probably be called
 *          moog_readline, but alas it is not.
 * 
 *
 * -Scott Swindell September 2019
 *
 *
 * ****************************************************/

int moog_read( int rs485_fd, char resp[] )
{
	int rn=1;
	int ii=0;
	fd_set set;
	struct timeval timeout;
	timeout.tv_sec = 0;
	timeout.tv_usec = 200000;
	FD_ZERO(&set);
	FD_SET(rs485_fd, &set);
	

	while(rn > 0)
	{
		if(select( rs485_fd+1, &set, NULL, NULL, &timeout ) == 1)
		{
			rn = read(rs485_fd, resp+ii, 1);
			//fprintf(stderr, "%i %c\n", ii, resp[ii]);
			if(resp[ii] == '\r' || resp[ii] == '\n')
			{
				resp[ii+1] = '\0';
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

	char msg[20];
	if( can_addr == -1 )//head node
		snprintf( msg, 20, "GOSUB(%i)", subnum );
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

	moog_write(rs485_fd, "ddd=0" ); // Turn debug off
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
 * 		100  for the filter wheels it is 
 * 		101. 
 *
 *
 * **************************************************/
int moog_home( int rs485_fd, int can_addr )
{
	switch(can_addr)
	{
		case -1: //Home all in series
			moog_callsub( rs485_fd, 100, can_addr );
		break;
		case OFFSET_X:
		case OFFSET_Y:
		case OFFSET_FOCUS:
		case OFFSET_MIRRORS:
			moog_callsub( rs485_fd, 103, can_addr );
		break;
		case OFFSET_FWHEEL:
		case FWHEEL_UPPER:
		case FWHEEL_LOWER:
			moog_callsub( rs485_fd, 103, can_addr );
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
 *       the position target 'PT' on the motor
 *       and then sending go command 'GOSUB(500)'
 *       
 *
 * **************************************************/
int moog_lgoto(int rs485_fd, int can_addr, int pos )
{
	char msg[40];
	snprintf( msg, 40, "PT:%i=%i", can_addr, pos );
	moog_write(rs485_fd, (const char *) msg);
	usleep(100); // this is probably not necessary
	//snprintf(msg, 40, "G:%i", can_addr );
	//moog_write(rs485_fd, (const char *) msg ); //GO!
	moog_callsub(rs485_fd, 500, can_addr);

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
	char msg[20];
	char resp[READSIZE];
	int error;

	moog_read(rs485_fd, resp);//flush--probably unecessary

	snprintf(msg, 20, "RPA:%i", stat->motor_num);
	moog_write(rs485_fd, (const char *) msg );
	if( moog_read(rs485_fd, resp) == 0)
	{
		stat->pos = atoi(resp);
	}
	else
		return -1;
	
	snprintf(msg, 20, "Rf:%i", stat->motor_num);
	moog_write(rs485_fd, (const char *) msg );
	if( moog_read(rs485_fd, resp) == 0)
	{
		stat->fnum = atoi(resp);
	}
	else
		return -1;

	for(int ii=0; ii<4; ii++)
	{
		snprintf( msg, 20, "RW(%i):%i", ii, stat->motor_num );
		moog_write( rs485_fd, (const char *) msg );
		error = moog_read(rs485_fd, resp);
		stat->words[ii] = atoi(resp);

	}

	snprintf( msg, 20, "RW(16):%i", stat->motor_num );
	moog_write(rs485_fd, (const char *) msg );
	if( moog_read(rs485_fd, resp) == 0)
    {
		stat->iobits = atoi(resp);
    }
	else
	{
		fprintf(stderr, "Could not determine homed status of %s check msg %s\n", stat->name, msg);
		return -1;
	}


	snprintf( msg, 20, "RW(12):%i", stat->motor_num );
	moog_write(rs485_fd, (const char *) msg );
	if( moog_read(rs485_fd, resp) == 0)
    {
		stat->userbits = atoi(resp);
        stat->isHomed = stat->userbits & 1;
    }
	else
	{
		fprintf(stderr, "Could not determine homed status of %s check msg %s\n", stat->name, msg);
		return -1;
	}


	switch(stat->motor_num)
	{
		case OFFSET_FWHEEL:
			stat->isFilter = 1;
		break;	
		case FWHEEL_LOWER:
			stat->isFilter = 1;
		break;
		case FWHEEL_UPPER:
			stat->isFilter = 1;
		break;
		default:
			stat->isFilter = 0;

	}
	return 0;
}

void moog_serialfix(int moogfd)
{
	
	char serialfix=128; 
	moog_write(moogfd, &serialfix);
}

/***************************************
 *Name: moog_getallstatus
 *Args: rs485_fd stat-> array of MSTATUS structs
 *Description: get the status of all the drives
 *		By querying individual motors one at a 
 *		time. This is a laborious process and 
 *		should be replaced. 
 *
 *
 *
 *
 *
 * ***************************************/
int moog_getallstatus(int rs485_fd, MSTATUS stat[])
{
	int active;
	char msg[10];
	char resp[READSIZE];
	snprintf(msg, 10, "RW(12)");
	moog_write(rs485_fd, (const char *) msg );
	if( moog_read(rs485_fd, resp) == 0)
	{
		active = atoi(resp);
	}
	else
	{
		fprintf(stderr, "WE could not get active status\n");
		return -1;
	}

	for(MSTATUS *motor=stat; motor!=stat+NMOTORS; motor++)
	{
		
		
		if((1<<motor->motor_num) & active)
		{
			//TODO error check this response.
			moog_getstatus(rs485_fd, motor);
			motor->isActive = 1;
		}
		else
			motor->isActive = 0;
	
	}
	return active;
}

/*****************************************************8
 *Name: moog_getallstatus_quick
 *Args: rs485_fd, motors->allmotors array
 *
 *Description: This function replaces the moog_getallstatus
 *	with a quicker version. Here we call the 998 subroutine
 *	on the head node and it spits back the necessary info
 *	about each motor all at once rather that querying 
 *	each piece of data one at a time. 
 *
 *
 *
 * *****************************************************/
int moog_getallstatus_quick(int rs485_fd, MSTATUS motors[])
{
	char resp[5000];
	int read_status=0;
	int motor_num=0, pos, f, w0, w1, w2, w3, userbits, iobits;
	int wc, active;
	static int foo = 0;
	int motor_count=0;
	moog_read(rs485_fd, resp);//Flush the line

	/* Something is causing the motors to not respond
	 * when this happens we call the serialfix. At first
	 * we were only doing this when the connection was
	 * oponed. We are having a problem with the current
	 * INDI driver where the motor does this a lot more
	 * hopefully this is a temporary fix while we leave
	 * find the cause of the motors not responding. 
	 * --Scott 10/2019
	 * */
	moog_callsub( rs485_fd, 998, -1);
	while(motor_num <7 )
	{
		moog_read(rs485_fd, resp);
		wc = sscanf(resp, "%i %i %i %i %i %i %i %i %i", &motor_num, &pos, &f, &w0, &w1, &w2, &w3, &userbits, &iobits );
		//fprintf(stderr, "[%s]\n", resp );
		if (wc != 9)
		{
			fprintf(stderr, "[%s]\n", resp );
			moog_serialfix(rs485_fd);
			continue;
		}
		else
		{
			fprintf(stderr, "{%s}\n", resp );
		}
		//fprintf(stderr, " %i %i %i %i %i %i %i %i %i\n", motor_num, pos, f, w0, w1, w2, w3, userbits, iobits );
		for(MSTATUS *motor=motors; motor!=motors+NMOTORS; motor++)
		{
			if(motor_num == motor->motor_num)
			{
				motor->pos = pos;
				motor->fnum = f;
				motor->words[0] = w0;
				motor->words[1] = w1;
				motor->words[2] = w2;
				motor->words[3] = w3;
				motor->userbits = userbits;
				motor->iobits = iobits;
				motor->isActive = 1;
				motor->inPosLimit = w0 & (1<<16);
				motor->inNegLimit = w0 & (1<<15);
				motor->isMoving = w0 & (1<<2);
				motor->isHomed = userbits & 1;
				active = (1<<motor->motor_num) | active;
				motor_count++;
			}
		}
	}

	if(motor_count != 7)
	{
		
		fprintf(stderr, "We are missing motors\n");
	}
	fprintf(stderr, "END getallstatus %i \n", active );
	return active;
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
	printf("  Pos == %i\n", stat.pos);
	printf("  fnum == %i\n", stat.fnum);
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
 * 		Uses the GOSUB(999) subroutine call
 * 		to get a list of all the motor names 
 * 		and their numbers from from the serial 
 * 		line. This information is then stored 
 * 		in the array of status structs. This
 * 		is how the GUI maps the can address 
 *		(motor_num) to the name of the motor.
 *
 *		TODO: The number of motors should 
 *		probably be a variable and not a 
 *		hard coded thing. Perhaps a macro 
 *		is called for. 
 *
 *
 *
 * ***************************/
int build_stat_structs( int rs485_fd, MSTATUS motors[] )
{
	char resp[READSIZE];
	char msg[20];
	moog_read(rs485_fd, resp); //flush line
	fprintf(stderr, "\nflush gives %s\n", resp);
	moog_read(rs485_fd, resp); //flush line
	fprintf(stderr, "flush gives %s\n", resp);
	int wc;
	int head_node;

	//Grab the head node address.
	moog_write(rs485_fd, "RCADDR");
	moog_read(rs485_fd, resp);
	head_node = atoi(resp);

	moog_callsub(rs485_fd, 999, -1 );
	for(int ii=0; ii<7; ii++ )
	{
		//TODO: We need to come up with a way to 
		//determine if the response back from the motor
		//is reliable. If a motor is not communicating 
		//on the can bus (as long as its not the head),
		//the response will be normal. This will require
		//a change to the smartmotor program.
		moog_read( rs485_fd, resp );
		fprintf(stderr, "%i %s\n", ii, resp);
		wc = sscanf(resp, "MOTOR #%i %s", &motors[ii].motor_num, motors[ii].name );
		motors[ii].head_node = head_node;
		if( strcmp(motors[ii].name, "OFFSET_FWHEEL") == 0 )
		{
			motors[ii].isFilter = 1;
		}
		else if( strcmp(motors[ii].name, "OFFSET_FWHEEL") == 0 )
		{
			motors[ii].isFilter = 1;
		}
		else if( strcmp(motors[ii].name, "OFFSET_FWHEEL") == 0 )
		{
			motors[ii].isFilter = 1;
		}
		else
		{
			motors[ii].isFilter = 0;
		}

	}

	for( MSTATUS *motorii=motors; motorii!=motors+7; motorii++ )
	{	
		if(motorii->motor_num < 1 || motorii->motor_num > 7 )
		{
			continue;// Erroneous motor num
		}
		
		//positive software limit
		snprintf( msg, 20, "RSLP:%i", motorii->motor_num );
		moog_write( rs485_fd, msg );
		moog_read( rs485_fd, resp );
		motorii->pos_slimit = atoi(resp);

		//negative software limit
		snprintf( msg, 20, "RSLN:%i", motorii->motor_num );
		moog_write( rs485_fd, msg );
		moog_read( rs485_fd, resp );
		motorii->neg_slimit = atoi(resp);


		//filter_dist is stored in hh variable.
		snprintf( msg, 20, "Rhh:%i", motorii->motor_num ); 
		moog_write( rs485_fd, msg );
		moog_read( rs485_fd, resp );
		motorii->fdist = atoi(resp);
		
	}
}


MSTATUS * motor_by_name(char name[], MSTATUS motors[] )
{
	MSTATUS * motor;
	for(motor=motors; motor<motors+7; motors++)
		if(strcmp( motor->name, name ) == 0)
			return motor;
}




