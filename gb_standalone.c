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

 #include "gb_commands.h"

void domessage(char *av0, char *message);
int doTelemetry(int port__fd);

/*############################################################################
#  Title: main
#  Author: C.Johnson
#  Date: 9/4/19
#  Args:  (int argc, char ** argv)
#  Description: uses command line options to figure out how to run the program
#
#############################################################################*/
int main(int argc, char ** argv)
{
	char strPort[100], strAxis[20];
	int doinit=0, dohome=0, domove=0, dohelp=0;
	int value=0, opt, iaxis, port_fd, isFilter=0;
	int axisinit=0, portinit=0, valinit=0, dotelem=0;

	while ((opt = getopt(argc, argv, "p:ihmta:tv:?")) != -1) 
		{
               	switch (opt) 
			{
               		case 'p': //Set Port
                   		sprintf(strPort, "%s", optarg);
                   		portinit = 1;
                   		break;
               		case 'i': //Initialize
                   		doinit = 1;
                  		break;
               		case 'h': //home
                   		dohome = 1;
                  		break;
               		case 'm': //home
                   		domove = 1;
                  		break;
               		case 'a': //Set Axis
                   		sprintf(strAxis, "%s", optarg);
				axisinit = 1;
                   		break;
               		case 'v': //Set Value
                   		value=atoi(optarg);
				valinit = 1;
                   		break;
               		case 't': //get telemetry
                   		dotelem = 1;
                   		break;
               		case '?': //HELP!!!
                   		dohelp=1;
				break;
               		default: /* '?' */
				domessage(argv[0], NULL);
           		exit(EXIT_FAILURE);
               		}
           	}

	//print the help
	if (dohelp)
		{
		domessage(argv[0], "lookin for help?");
		}

	//no port specified...  do not continue
	if(!portinit)
		{
		domessage(argv[0], "ERROR!!!  Must specify valid port");
			
		}
	
	//check and see if port opens
	port_fd = open_port( strPort );
	if(port_fd == -1)
		{
		fprintf(stderr, "\nError Opening Port %s\n", strPort);
		exit(EXIT_FAILURE);
		}        

	
	//initialize motor chain on [port]
	if (doinit)
		{
		printf("INITIALIZING!!!\n");
		guider_init( port_fd );
		close_port( port_fd );
		//port_init( 0, NULL, port_fd );
		exit(0);
		}

	//grab telemetry
	if(dotelem)
		{
		doTelemetry(port_fd);
		close_port( port_fd );
		exit(0);
		}
	
	//validate [axis] before we proceed
	if(!axisinit)
		{
		close_port( port_fd );
		domessage(argv[0], "ERROR!!!  Must specify valid axis");
		
		}
	
	iaxis = validateAxis(strAxis, &isFilter);
	if(!iaxis)
		{
		close_port( port_fd );
		domessage(argv[0], "ERROR!!!  invalid axis");
			
		}

	
	//home axis [axis]
	else if (dohome)
		{
		printf("HOMING %s!!!\n", strAxis);
		moog_home( port_fd, iaxis );
		close_port( port_fd );
		exit(0);
		}

	//move axis [axis] to position [value]
	else if (domove)
		{
		if((!axisinit)||(!valinit))
			{
			close_port( port_fd );
			domessage(argv[0], "ERROR!!!  Must specify valid axis and value");
			}
		printf("MOVING %s to %i!!!\n", strAxis, value);
		if (strcmp(strAxis, "OFFSET_X")==0)
			offsetXGoTo(port_fd, value);
		else if (strcmp(strAxis, "OFFSET_Y")==0)
			offsetYGoTo(port_fd, value);
		else if (strcmp(strAxis, "OFFSET_FOCUS")==0)
			offsetFocusGoTo(port_fd, value);
		else if (strcmp(strAxis, "OFFSET_MIRRORS")==0)
			offsetMirrorsGoTo(port_fd, value);
		else if (strcmp(strAxis, "OFFSET_FWHEEL")==0)
			offsetFilterGoTo(port_fd, value);
		else if (strcmp(strAxis, "FWHEEL_LOWER")==0)
			lowerFilterGoTo(port_fd, value);
		else if (strcmp(strAxis, "FWHEEL_UPPER")==0)
			upperFilterGoTo(port_fd, value);
		}
	else
		{
		domessage(argv[0], "ERROR!!!  Unknown Command");
		}
	close_port( port_fd );
		

}

/*############################################################################
#  Title: domessage(char *av0, char *message)
#  Author: C.Johnson
#  Date: 9/4/19
#  Args:  char *av0 = pointer to name of command issued to start program
#	char *message = optional message to append to beginning
#  Description: prints the help message
#
#############################################################################*/
void domessage(char *av0, char *message)
{
	if(message != NULL)
		fprintf(stderr, "\n%s\n", message);

	fprintf(stderr, "\nVATT Guide Box Command Interface v.42\n");
        fprintf(stderr, "code name: Don't Panic!\n\n");
        fprintf(stderr, "Usage: %s -r -p[port] -m -a[axis] -v[value]\n",av0);
        fprintf(stderr, "    -p: set port to [port]  exe. /dev/ttyUSB0\n");
        fprintf(stderr, "    -i: initialize [wont do any additional actions]\n");
        fprintf(stderr, "    -h: home axis [axis]\n");
        fprintf(stderr, "    -m: move axis [axis]to value [value]\n");
        fprintf(stderr, "    -a: set axis to [axis]  exe. FWHEEL_LOWER\n");
        fprintf(stderr, "    -v: set axis value to [value]  exe. 200\n");
        fprintf(stderr, "    -?: print this help message\n");
        fprintf(stderr, "\nValid Axis:\n");
        fprintf(stderr, "    OFFSET_X\n");
        fprintf(stderr, "    OFFSET_Y\n");
        fprintf(stderr, "    OFFSET_FOCUS\n");
        fprintf(stderr, "    OFFSET_MIRRORS\n");
        fprintf(stderr, "    OFFSET_FWHEEL\n");
        fprintf(stderr, "    FWHEEL_LOWER\n");
        fprintf(stderr, "    FWHEEL_UPPER\n");
        fprintf(stderr, "\n");
	
	exit(EXIT_FAILURE);
               	

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

/****************************************************
 * Name: main_old
 * Author: Scott Swindell
 * Date 7/20/2019
 * desc: original main function by Scott Swindell
 *    keeping here for historic purposes
 * 
 *********************************************/

 int main2(int argc, char ** argv)
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

	//moog_home( fd, OFFSET_Y );
	/*Wait 3 seconds till home
	 * we should instead check the 
	 * is-homed bit, which is not implemented yet
	 *
	 * */
	//sleep(3);
	
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
