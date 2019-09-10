/*############################################################################
#  Title: gb_commands.c
#  Author: Chris Johnson
#  Date: 9/4/19
#  Description: commands for guide box..  abstracted from serial commands
#	for easy use by command line, indi driver, and ng protocol
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

 #include "gb_commands.h"

//MSTATUS allmotors[7];

/*############################################################################
#  Title: stageHome
#  Author: C.Johnson
#  Date: 9/9/19
#  Args:  int ttyfd -> tty port file descriptor
#	  char *stage -> string containing name of stage
#  Returns: ???
#  Description: send stage to home position...  not yet fully implemented
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
#  Args:  char *ttyport -> string containing name of port to use(/dev/ttyUSB0)
#  Returns: file descriptor of open tty port
#  Description: opens the tty port referred to by ttyPort and returns a file
#	descriptor
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
#  Returns: N/A
#  Args:  int ttyfd -> tty port file descriptor
#  Description: closes the ttyport referred to by the file descriptor ttyfd
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
#  Args:  int ttyfd -> tty port file descriptor
#	  char *strAxis -> string containing name of stage
#	  int position -> new position
#  Returns: on success, integer value of axis(non-zero).  0 on failure
#  Description: sents the stage named strAxis to new position "position"
#
#############################################################################*/
 int stageGoTo (int ttyfd, char *strAxis, int POSITION)
 { 
  int isFilter=0, iAxis;
	iAxis = validateAxis(strAxis, &isFilter);
	if (iAxis < 1)
		{
		return 0;
		}
	if (isFilter)
		{
		moog_fgoto(ttyfd, iAxis, POSITION);
		}
       else
		{
		moog_lgoto(ttyfd, iAxis, POSITION);
		}

	printf("got axis %i:%s isfilter=%i\n", iAxis, strAxis, isFilter);
       return iAxis;

        	
 }


/*############################################################################
#  Title: guider_init( char usbport[] )
#  Author: C.Johnson
#  Date: 9/3/19
#  Args:  int ttyfd -> tty port file descriptor
#  Returns: ???
#  Description: sends the init string to the head node.  this inits all
#	motors on the bus.
#
#############################################################################*/
int guider_init( int ttyfd )
{
char resp[READSIZE];


	moog_read(ttyfd, resp);
	printf("moog_resp=%s\n", resp);
	moog_init( ttyfd );
	
	
}


/*############################################################################
#  Title: validateAxis(char *axis)
#  Author: C.Johnson
#  Date: 9/3/19
#  Args:  char *axis -> string of axis name
#	int *isFilter -> pointer to integer flag to say whether this is a filter.
#  Returns: on success, integer value of axis(non-zero).  0 on failure
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
#  Args:  int ttyfd -> tty port file descriptor
#  Returns: ???
#  Description: grabs telemetry from guider..  hack from S.Swindells original
#	example routine.
#
#############################################################################*/
int doTelemetry(int ttyfd, MSTATUS *allmotors)
{
char resp[200];
int active, x;

printf("pretending to do telemetry\n");
	//TODO this should not be called every time
	//It should be called once at the beginning of the program.
	build_stat_structs( ttyfd, allmotors );
	moog_write( ttyfd, "RW(12)"  ); //user bits that show which motors are active
	x=moog_read( ttyfd, resp );
	if(x>0)
		printf("moog_response = %s\n", resp);
	else
		printf("no_response\n");
	
	active = atoi(resp);

	for(int num=1; num<=7; num++)
	{
		if(active & (1<<num))
		{
			moog_getstatus(ttyfd, &allmotors[num-1]);
			allmotors[ num-1 ].isActive = 1;
			//print_status( allmotors[ num-1 ] );
		}
		else
		{
			allmotors[ num-1 ].isActive = 0;
		}

	}

}
