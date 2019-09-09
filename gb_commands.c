/*############################################################################
#  Title: gb_commands.c
#  Author: Chris Johnson
#  Date: 9/4/19
#  Description: commands for guide box
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
 #include "gb_commands.h"

MSTATUS allmotors[7];



/*############################################################################
#  Title: stageHome
#  Author: C.Johnson
#  Date: 9/9/19
#  Args:  N/A
#  Description: 
#
#############################################################################*/
 int stageHome (int ttyfd, char *stage)
 { 
 return 1;
      
 }

/*############################################################################
#  Title: ttyOpen
#  Author: C.Johnson
#  Date: 9/9/19
#  Args:  N/A
#  Description: 
#
#############################################################################*/
 int ttyOpen (char *ttyPort)
 { 
 int ttyfd;
  
	ttyfd = open_port( ttyPort );
	return ttyfd;
      
 }

/*############################################################################
#  Title: ttyClose
#  Author: C.Johnson
#  Date: 9/9/19
#  Args:  N/A
#  Description: 
#
#############################################################################*/
 void ttyClose (int ttyfd)
 { 
  close_port( ttyfd );

        	
 }



/*############################################################################
#  Title: stageGoTo
#  Author: C.Johnson
#  Date: 9/9/19
#  Args:  N/A
#  Description: 
#
#############################################################################*/
 int stageGoTo (int port_fd, char *strAxis, int POSITION)
 { 
  int isFilter=0, iAxis;
	iAxis = validateAxis(strAxis, &isFilter);
	if (iAxis < 1)
		{
		return 0;
		}
	if (isFilter)
		{
		moog_fgoto(port_fd, iAxis, POSITION);
		}
       else
		{
		moog_lgoto(port_fd, iAxis, POSITION);
		}
       return iAxis;

        	
 }


/*############################################################################
#  Title: guider_init( char usbport[] )
#  Author: C.Johnson
#  Date: 9/3/19
#  Args:  N/A
#  Description: 
#
#############################################################################*/
int guider_init( int port_fd )
{
char resp[READSIZE];


	moog_read(port_fd, resp);
	printf("moog_resp=%s\n", resp);
	moog_init( port_fd );
	//build_stat_structs(port_fd, allmotors); //Map names to numbers
	
	
}

/*############################################################################
#  Title: port_init( int open, char usbport[], int port_fd )
#  Author: C.Johnson
#  Date: 9/6/19
#  Args:  N/A
#  Description: 
#
#############################################################################*/
int port_init( int open, char usbport[], int port_fd )
{


	if (open)
		port_fd = open_port( usbport );	
	else
		close_port(port_fd);


	return port_fd;
	
}



/*############################################################################
#  Title: validateAxis(char *axis)
#  Author: C.Johnson
#  Date: 9/3/19
#  Args:  char *axis -> string of axis name
#	int *isFilter -> pointer to integer flag to say whether this is a filter.
#  Description: validates the axis string and returns an appropriate port int
#    otherwise returns 0
#
#############################################################################*/
int validateAxis(char *axis, int *isFilter)
{
int iaxis;
if (strcmp(axis, "OFFSET_X")==0){
	iaxis=OFFSET_X;
	*isFilter=0;}
else if (strcmp(axis, "OFFSET_Y")==0){
	iaxis=OFFSET_Y;
	*isFilter=0;}
else if (strcmp(axis, "OFFSET_FOCUS")==0){
	iaxis=OFFSET_FOCUS;
	*isFilter=0;}
else if (strcmp(axis, "OFFSET_MIRRORS")==0){
	iaxis=OFFSET_MIRRORS;
	*isFilter=0;}
else if (strcmp(axis, "OFFSET_FWHEEL")==0){
	iaxis=OFFSET_FWHEEL;
	*isFilter=1;}
else if (strcmp(axis, "FWHEEL_LOWER")==0){
	iaxis=FWHEEL_LOWER;
	*isFilter=1;}
else if (strcmp(axis, "FWHEEL_UPPER")==0){
	iaxis=FWHEEL_UPPER;
	*isFilter=1;}
else 
	iaxis = 0;

return iaxis;
}

/*############################################################################
#  Title: doTelemetry
#  Author: C.Johnson
#  Date: 9/4/19
#  Args:  
#  Description: grabs telemetry from guider
#
#############################################################################*/
int doTelemetry(int port_fd)
{
char resp[200];
int active, x;

printf("pretending to do telemetry\n");
	moog_write( port_fd, "RW(12)"  ); //user bits that show which motors are active
	x=moog_read( port_fd, resp );
	if(x>0)
		printf("moog_response = %s\n", resp);
	else
		printf("no_response\n");
	
	active = atoi(resp);

	for(int num=1; num<9; num++)
	{
		if(active & (1<<num))
		{
			moog_getstatus(port_fd, &allmotors[num-1]);
			print_status( allmotors[ num-1 ] );
		}
	}

}
