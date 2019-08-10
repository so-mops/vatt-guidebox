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



/*
 * 'open_port()' - Open serial port 1.
 *
 * Returns the file descriptor on success or -1 on error.
 */

int open_port( char usbport[] )
{
	int fd; /* File descriptor for the port */


	fd = open(usbport, O_RDWR | O_NOCTTY | O_NDELAY);
	fcntl(fd, F_SETFL, FNDELAY);
	if (fd == -1)
	{
		//Could not open the port.
		perror("open_port: Unable to open port");
	}

	else
		fcntl(fd, F_SETFL, 0);

	return (fd);
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
			//printf("%i %c\n", ii, resp[ii]);
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
	
	char resp[100];
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


int moog_fgoto( int rs485_fd, int can_addr, int fnum )
{
	char msg[10];
    snprintf( msg, 10, "f:%i=%i", can_addr, fnum );
	moog_write(rs485_fd, (const char *) msg);
	moog_callsub(rs485_fd, 400, can_addr);//call the subroutine to move fwheel
	usleep(100); // this is probably not necessary
	//moog_callsub(rs485_fd, )
}

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



int main(int argc, char ** argv)
{
	MSTATUS allmotors[7];
	char resp[100];	
	char buf[10];
	int active;

	if( argc != 2 )
	{
		printf( "\nUsage: %s <usbport>\n Like:%s /dev/ttyUSB0\n", argv[0], argv[0] );
		exit(2);
	}

	int fd = open_port( argv[1] );
	if(fd == -1)
		exit(-1);

	moog_read(fd, resp);
	moog_init( fd );
	build_stat_structs(fd, allmotors); //Map names to numbers

	moog_write( fd, "RW(12)"  ); //user bits that show which motors are active
	moog_read( fd, resp );
	active = atoi(resp);

	for(int num=1; num<9; num++)
	{
		if(active & (1<<num))
		{
			moog_getstatus(fd, &allmotors[num-1]);
			print_status( allmotors[ num-1 ] );
		}
	}

	moog_home( fd, OFFSET_Y );
	/*Wait 3 seconds till home
	 * we should instead check the 
	 * is-homed bit, which is not implemented yet
	 *
	 * */
	sleep(3);
	
	// move the offset Y axis to 10000
	moog_lgoto(fd, OFFSET_Y, 10000);
	sleep(3);
	
	// move guider fwheel to filter #5
	moog_fgoto(fd, OFFSET_FWHEEL, 5 );
	sleep(3);


	// move lower fwheel to filter #5
	moog_fgoto(fd, FWHEEL_LOWER, 5 );
	sleep(3);


	close( fd );


}
