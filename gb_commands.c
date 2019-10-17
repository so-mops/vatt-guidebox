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
 if (stage == NULL)
 {
	 moog_home(ttyfd, -1);
 } 
 return 1;
      
 }

/*############################################################################
#  Title: net_ttyOpen
#  Author: C.Johnson
#  Date: 10/1/19
#  Args:  char *netinfo -> string containing address and (optionally) port
#  Returns: file descriptor of open socket
#  Description: parses the string pointed to by netinfo.  if a ":" exists, it
#	will use that to delimit address:port.  otherwise it will just
#	assume its an address.  This will call open_port_net and get an file
#	descriptor to return.  Intended for use with a lantronix
#
#############################################################################*/
int net_ttyOpen (char *netinfo)
{ 
int port=10001, netfd;
char input[50], *address, *portstr;

	//so we don't destroy the original string    
	sprintf(input, "%s", netinfo);
	//sprintf(input, "10.130.133.24:10001");
 
	//get the address
	address = strtok(input, ":"); 
	if (address == NULL)
		{
		return 0;
		}
	//get the port(if it exists or is valid
	portstr = strtok(NULL, ":"); 
	if (portstr != NULL)
		{
		port=atoi(portstr);
		if (port==0)
			port=10001;
		}
	fprintf(stderr, "opening add:%s  port%i\n", address, port);
	netfd = open_port_net( address, port );
	return netfd; 
    
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
#  Title: guiderRead
#  Author: Scott Swindell
#  Date: 9/20/19
#  Returns: 0 on success -1 if there is nothing to read.
#  Args:  int ttyfd -> tty port file descriptor
#		resp -> response to be filled on read
#  Description: closes the ttyport referred to by the file descriptor ttyfd
#
#############################################################################*/

 int guiderRead(int ttyfd, char * resp)
 {
 	return moog_read(ttyfd, resp);
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
int doTelemetry(int ttyfd, MSTATUS *allmotors, int init_struct)
{
char resp[200];
int active, x;


	//It should be called once at the beginning of the program.
	if(init_struct)
	{
		fprintf(stderr, "starting build_stat_structs");
		build_stat_structs( ttyfd, allmotors );
	}

	
	return moog_getallstatus_quick(ttyfd, allmotors);
}

/*############################################################################
#  Title: reset_net_adapter()
#  Author: C.Johnson
#  Date: 10/2/19
#  Args:  char * host -> address of lantronix
	int port -> port for telnet interface on lantronix
#  Returns: ???
#  Description: resets lantronix adapter.
#
#############################################################################*/
int lantronix_reset(char *netinfo)
{
int port=10001, netfd;
char input[50], *address, *portstr;
		
	//so we don't destroy the original string    
	sprintf(input, "%s", netinfo);
	
	//get the address
	address = strtok(input, ":"); 
	if (address == NULL)
		{
		printf("no address\n");
		return 0;
		}
	
	gb_lantronix_reset(address, 9999);
}
