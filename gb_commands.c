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

 #include "gb_commands.h"


/*############################################################################
#  Title: offsetXGoTo
#  Author: C.Johnson
#  Date: 8/20/19
#  Args:  N/A
#  Description: 
#
#############################################################################*/
 void offsetXGoTo (int port_fd, int POSITION)
 { 
       moog_lgoto(port_fd, OFFSET_X, POSITION);

	//handle setting motion status here	  
         
 }

/*############################################################################
#  Title: offsetYGoTo
#  Author: C.Johnson
#  Date: 8/20/19
#  Args:  N/A
#  Description: 
#
#############################################################################*/
 void offsetYGoTo (int port_fd, int POSITION)
 { 
       moog_lgoto(port_fd, OFFSET_Y, POSITION);

	//handle setting motion status here	  
         
 }

/*############################################################################
#  Title: offsetFocusGoTo
#  Author: C.Johnson
#  Date: 8/20/19
#  Args:  N/A
#  Description: 
#
#############################################################################*/
 void offsetFocusGoTo (int port_fd, int POSITION)
 { 
       moog_lgoto(port_fd, OFFSET_FOCUS, POSITION);

	//handle setting motion status here	  
         
 }

/*############################################################################
#  Title: offsetMirrorsGoTo
#  Author: C.Johnson
#  Date: 8/20/19
#  Args:  N/A
#  Description: 
#
#############################################################################*/
 void offsetMirrorsGoTo (int port_fd, int POSITION)
 { 
       moog_lgoto(port_fd, OFFSET_MIRRORS, POSITION);

	//handle setting motion status here	  
         
 }

/*############################################################################
#  Title: offsetFilterGoTo
#  Author: C.Johnson
#  Date: 8/20/19
#  Args:  N/A
#  Description: 
#
#############################################################################*/
 void offsetFilterGoTo (int port_fd, int POSITION)
 { 
         moog_fgoto(port_fd, OFFSET_FWHEEL, POSITION); 

         //handle setting motion status here	
 }

/*############################################################################
#  Title: lowerFilterGoTo
#  Author: C.Johnson
#  Date: 8/20/19
#  Args:  N/A
#  Description: 
#
#############################################################################*/
 void lowerFilterGoTo (int port_fd, int POSITION)
 { 
         moog_fgoto(port_fd, FWHEEL_LOWER, POSITION); 

         //handle setting motion status here	
 }

/*############################################################################
#  Title: upperFilterGoTo
#  Author: C.Johnson
#  Date: 8/20/19
#  Args:  N/A
#  Description: 
#
#############################################################################*/
 void upperFilterGoTo (int port_fd, int POSITION)
 { 
         moog_fgoto(port_fd, FWHEEL_UPPER, POSITION); 

         //handle setting motion status here	
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
