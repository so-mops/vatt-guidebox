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
 void offsetXGoTo (int RS485_FD, int POSITION)
 { 
       moog_lgoto(RS485_FD, OFFSET_X, POSITION);

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
 void offsetYGoTo (int RS485_FD, int POSITION)
 { 
       moog_lgoto(RS485_FD, OFFSET_Y, POSITION);

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
 void offsetFocusGoTo (int RS485_FD, int POSITION)
 { 
       moog_lgoto(RS485_FD, OFFSET_FOCUS, POSITION);

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
 void offsetMirrorsGoTo (int RS485_FD, int POSITION)
 { 
       moog_lgoto(RS485_FD, OFFSET_MIRRORS, POSITION);

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
 void offsetFilterGoTo (int RS485_FD, int POSITION)
 { 
         moog_fgoto(RS485_FD, OFFSET_FWHEEL, POSITION); 

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
 void lowerFilterGoTo (int RS485_FD, int POSITION)
 { 
         moog_fgoto(RS485_FD, FWHEEL_LOWER, POSITION); 

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
 void upperFilterGoTo (int RS485_FD, int POSITION)
 { 
         moog_fgoto(RS485_FD, FWHEEL_UPPER, POSITION); 

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
int guider_init( int act, char *usbport )
{
int fd; /* File descriptor for the port */
char resp[READSIZE];


	if(act != 1)
		{
		close_port( RS485_FD );
		RS485_FD = -1;
		return -1;
		}
	fd = open_port( usbport );
	if(fd != -1)
		{
		moog_read(fd, resp);
		moog_init( fd );
		build_stat_structs(fd, allmotors); //Map names to numbers
		}
	return (fd);
	
}
