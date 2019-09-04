/*############################################################################
#  Title: gb-standalone.c
#  Author: Chris Johnson
#  Date: 9/3/19
#  Description: standalone functionality for guide box
#
#
#############################################################################*/ 

 /* Standard headers */
 #include <stdio.h>
 #include <stdlib.h>
 #include <string.h>
 #include <math.h>
 #include <unistd.h>
 #include <sys/time.h>
 
 #include <sys/time.h>
 #include <time.h>

 #include "gb_serial.h"



/****************************************************
 * Name: main
 * Author Scott Swindell
 * Date 7/20/2019
 * 
 *********************************************/

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
