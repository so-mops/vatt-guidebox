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
void displayTelem(int ttyfd);


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
	int doinit=0, dohome=0, domove=0, dohelp=0, reset_ltx=0;
	int value=0, opt, iaxis, ttyfd, isFilter=0, test, isnet=0;
	int axisinit=0, portinit=0, valinit=0, dotelem=0;
	char resp[10000];
	while ((opt = getopt(argc, argv, "p:ihLmta:tv:?")) != -1) 
		{
               	switch (opt) 
			{
               		case 'p': //Set Port
                   		sprintf(strPort, "%s", optarg);
                   		portinit = 1;
                   		break;
			case 'n': //set as network
				isnet = 1;
               		case 'i': //Initialize
                   		doinit = 1;

                  		break;
               		case 'h': //home
                   		dohome = 1;
                  		break;
               		case 'L': //reset lantronix
                   		reset_ltx = 1;
                  		break;
               		case 'm': //move
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
	if(reset_ltx)
		{
		fprintf(stderr, "\nOpening Port %s\n", strPort);
		lantronix_reset(strPort);
		}
	//check and see if port opens
	if(!isnet)
		ttyfd = ttyOpen( strPort );
	else
		ttyfd = net_ttyOpen( strPort );

	if(ttyfd == -1)
		{
		fprintf(stderr, "\nError Opening Port %s\n", strPort);
		exit(EXIT_FAILURE);
		}        

	
	//initialize motor chain on [port]
	if (doinit)
		{
		printf("INITIALIZING!!!\n");
		guider_init( ttyfd );
		ttyClose( ttyfd );
		exit(0);
		}

	//grab telemetry
	if(dotelem)
		{
		displayTelem(ttyfd);
		ttyClose( ttyfd );
		exit(0);
		}
	
	//validate [axis] before we proceed
	if(!axisinit)
		{
		ttyClose( ttyfd );
		domessage(argv[0], "ERROR!!!  Must specify valid axis");
		
		}
	
	iaxis = validateAxis(strAxis, &isFilter);
	if(!iaxis)
		{
		ttyClose( ttyfd );
		domessage(argv[0], "ERROR!!!  invalid axis");
			
		}

	
	//home axis [axis]
	else if (dohome)
		{
		printf("HOMING %s!!!\n", strAxis);
		stageHome( ttyfd, strAxis );
		ttyClose( ttyfd );
		exit(0);
		}

	//move axis [axis] to position [value]
	else if (domove)
		{
		if((!axisinit)||(!valinit))
			{
			ttyClose( ttyfd );
			domessage(argv[0], "ERROR!!!  Must specify valid axis and value");
			}
		printf("MOVING %s to %i!!!\n", strAxis, value);
		test=stageGoTo(ttyfd, strAxis, value);
		printf("moved stage %i\n", test);
		}
	else
		{
		domessage(argv[0], "ERROR!!!  Unknown Command");
		}
	ttyClose( ttyfd );
		

}

/*############################################################################
#  Title: displayTelemetry(char *av0, char *message)
#  Author: C.Johnson
#  Date: 9/4/19
#  Args:  char *av0 = pointer to name of command issued to start program
#	char *message = optional message to append to beginning
#  Description: prints the help message
#
#############################################################################*/
void displayTelem(int ttyfd)
{
MSTATUS allmotors[7];

int ix,ix2;
	doTelemetry(ttyfd, allmotors, 1);
	for(ix=0;ix<7;ix++)
		{
		printf("******Stage %i*******\n", ix);
		printf("\tactive=%i\n",allmotors[ix].isActive);
		for(ix2=0;ix2<4;ix2++)
			{
			printf("\t\twords[%i]=%i\n", ix2, allmotors[ix].words[ix2]);
			}
		printf("\tuserbits=%i\n", allmotors[ix].userbits);
		printf("\tmotor_num=%i\n", allmotors[ix].motor_num);
		printf("\tpos=%i\n", allmotors[ix].pos);
		printf("\tname=%s\n", allmotors[ix].name);
		printf("\tfnum=%i\n", allmotors[ix].fnum);
		}
		
		
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
        fprintf(stderr, "Usage: %s -p[port] -m -a[axis] -v[value]\n",av0);
        fprintf(stderr, "    -p: set port to [port]  exe. /dev/ttyUSB0\n");
        fprintf(stderr, "    -i: initialize [ignores all other arguments]\n");
        fprintf(stderr, "    -h: home axis [axis]\n");
        fprintf(stderr, "    -m: move axis [axis]to value [value]\n");
        fprintf(stderr, "    -a: set axis to [axis]  exe. FWHEEL_LOWER\n");
        fprintf(stderr, "    -v: set axis value to [value]  exe. 200\n");
        fprintf(stderr, "    -t: get telemetry data\n");
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




