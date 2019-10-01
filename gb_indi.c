/*############################################################################
#  Title: gb-indi.c
#  Author: Chris Johnson
#  Date: 8/20/19
#  Description:   indi driver for vatt guider box
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

 /* INDI Core headers */
#include <libindi/indiapi.h>
#include <libindi/indidevapi.h>
#include <libindi/eventloop.h>
#include <libindi/indicom.h>



/* header for this project */
#include "gb_commands.h"
	


 /* Definitions */

#define TTYPORT "/dev/ttyUSB0"

#define mydev		"INDI-VATT-GUIDEBOX"
#define MAIN_GROUP	"Guider Control"                  /* Group name */
#define ENG_GROUP	"Engineering"

#define MOT1_GROUP "MOTOR 1 Eng"
#define MOT2_GROUP "MOTOR 2 Eng"
#define MOT3_GROUP "MOTOR 3 Eng"
#define MOT4_GROUP "MOTOR 4 Eng"
#define MOT5_GROUP "MOTOR 5 Eng"
#define MOT6_GROUP "MOTOR 6 Eng"
#define MOT7_GROUP "MOTOR 7 Eng"

#define POLLMS          1000                             /* poll period, ms */



int RS485_FD;
int inited;


static void gbIndiInit();
static void zeroTelem();
void guiderProc (void *p);
static void buildMStatusString(MSTATUS *, char *);
static void fillMotors();
		
//global memory for IText properties
char gttyPORT[20];
char gnetwork[20];
char gstatusString[7][1000];
// main connection switch
static ISwitch connectS[] = {
	   {"CONNECT",  "Connect",  ISS_OFF, 0, 0}
     //{"CONNECT",  "On",  ISS_OFF, 0, 0}, {"DISCONNECT", "Off", ISS_ON, 0, 0}
	 };
static ISwitchVectorProperty connectSP = { mydev, "CONNECTION", "Connection",  MAIN_GROUP, IP_RW, ISR_NOFMANY, 0, IPS_IDLE,  connectS, NARRAY(connectS), "", 0 };
 

//***Guider Telemetry Data***
struct stdT{
	char offfoc[20],ufw[20],lfw[20],offx[20],offy[20],ofw[20],offmirr[20];};

struct stdT stdTelem;

static IText stdTelemT[] = {{"foc", "Focus Position "       , stdTelem.offfoc, 0, 0, 0},
			{"xpos", "offset x "       , stdTelem.offx, 0, 0, 0},
			{"ypos", "offset y "       , stdTelem.offy, 0, 0, 0},
			{"ufw", "Upper Filter "       , stdTelem.ufw, 0, 0, 0},
			{"lfw", "Lower Filter "       , stdTelem.lfw, 0, 0, 0},
			{"ofw", "offset Filter "       , stdTelem.ofw, 0, 0, 0},
			{"offmirr", "offset mirror "       , stdTelem.offmirr, 0, 0, 0}};
			
static ITextVectorProperty stdTelemTP = {  mydev, "stTELEM", "Guider Telemetry",  MAIN_GROUP , IP_RO, 0, IPS_IDLE,  stdTelemT, NARRAY(stdTelemT), "", 0};

// connection type switch
static ISwitch comTypeS[] = {
	 {"NETWORK",  "Network",  ISS_ON, 0, 0},
         {"SERIAL",  "Serial",  ISS_OFF, 0, 0}
	 };
static ISwitchVectorProperty comTypeSP = { mydev, "COMTYPE", "Connection Type",  MAIN_GROUP, IP_RW, ISR_1OFMANY, 0, IPS_IDLE,  comTypeS, NARRAY(comTypeS), "", 0 };


static IText networkT[] =  {{"NETWORK", "Net", "10.130.133.24:10001", 0, 0, 0}};
static ITextVectorProperty networkTP = {  mydev, "NET", "Guider Network Information",  ENG_GROUP , IP_RW, 0, IPS_IDLE,  networkT, NARRAY(networkT), "", 0};

static IText ttyPortT[] =  {{"TTYport", "TTY Port", "/dev/ttyUSB0", 0, 0, 0}};
static ITextVectorProperty ttyPortTP = {  mydev, "COM", "Guider Communication",  ENG_GROUP , IP_RW, 0, IPS_IDLE,  ttyPortT, NARRAY(ttyPortT), "", 0};


static IText motorStatusT[] = {
	{"M1", "Motor 1", "", 0, 0, 0},
	{"M2", "Motor 2", "", 0, 0, 0},
	{"M3", "Motor 3", "", 0, 0, 0},
	{"M4", "Motor 4", "", 0, 0, 0},
	{"M5", "Motor 5", "", 0, 0, 0},
	{"M6", "Motor 6", "", 0, 0, 0},
	{"M7", "Motor 7", "", 0, 0, 0},
};
static ITextVectorProperty motorStatusTP = { mydev, "MSTATUS", "Motor Status", ENG_GROUP, IP_RO, 0, IPS_IDLE, motorStatusT, NARRAY(motorStatusT), "", 0};


static ISwitch engSwitchsS[] = {{"SLIMITS", "FIND SOFT LIMITS", ISS_OFF, NULL, 0}};
static ISwitchVectorProperty engSwitchSP = {mydev, "Switches", "Switches", ENG_GROUP, IP_RW, ISR_NOFMANY, 0, IPS_IDLE, engSwitchsS, NARRAY(engSwitchsS), "", 0};

/************************************************
Group:  MAIN
************************************************/
//guider actions
static ISwitch actionS[]  = {{"INITIALIZE",  "Initialize",  ISS_OFF, 0, 0},{"HOME",  "Home",  ISS_OFF, 0, 0}};

ISwitchVectorProperty actionSP      = { mydev, "GUIDE_BOX_ACTIONS", "Guide Box Actions",  MAIN_GROUP, IP_RW, ISR_NOFMANY, 0, IPS_IDLE,  actionS, NARRAY(actionS), "", 0 };



//Upper Filter Goto
static INumber ufwNR[] = {{"FWHEEL_UPPER","Upper Filter", "%f",0., 0., 0., 0., 0, 0, 0}, };

 static INumberVectorProperty ufwNPR = {  mydev, "FWHEEL_UPPER", "Upper Filter Wheel goto",  MAIN_GROUP , IP_RW, 0, IPS_OK,  ufwNR, NARRAY(ufwNR), "", 0};

//Lower Filter Goto
static INumber lfwNR[] = {{"FWHEEL_LOWER","Lower Filter", "%f",0., 0., 0., 0., 0, 0, 0}, };

 static INumberVectorProperty lfwNPR = {  mydev, "FWHEEL_LOWER", "Lower Filter Wheel goto",  MAIN_GROUP , IP_RW, 0, IPS_IDLE,  lfwNR, NARRAY(lfwNR), "", 0};

//Offset X Goto
static INumber offxNR[] = {{"OFFSET_X","Offset X Position", "%f",0., 0., 0., 0., 0, 0, 0}, };

 static INumberVectorProperty offxNPR = {  mydev, "OFFSET_X", "offset x goto",  MAIN_GROUP , IP_RW, 0, IPS_IDLE,  offxNR, NARRAY(offxNR), "", 0};

//Offset Y Goto
static INumber offyNR[] = {{"OFFSET_Y","Offset Y Position", "%f",0., 0., 0., 0., 0, 0, 0}, };

 static INumberVectorProperty offyNPR = {  mydev, "OFFSET_Y", "offset y goto",  MAIN_GROUP , IP_RW, 0, IPS_IDLE,  offyNR, NARRAY(offyNR), "", 0};

//Offset Focus Goto
static INumber offFocNR[] = {{"OFFSET_FOCUS","Offset Focus", "%f",0., 0., 0., 0., 0, 0, 0}, };

 static INumberVectorProperty offFocNPR = {  mydev, "OFFSET_FOCUS", "offset foc goto",  MAIN_GROUP , IP_RW, 0, IPS_IDLE,  offFocNR, NARRAY(offFocNR), "", 0};

//Offset Mirror Goto
static INumber offMirrNR[] = {{"OFFSET_MIRRORS","Offset Mirror Position", "%f",0., 90., 0., 0., 0, 0, 0}, };

static INumberVectorProperty offMirrNPR = {  mydev, "OFFSET_MIRRORS", "offset mirror goto",  MAIN_GROUP , IP_RW, 0, IPS_IDLE,  offMirrNR, NARRAY(offMirrNR), "", 0};

//Offset Filter Goto
static INumber ofwNR[] = {{"OFFSET_FWHEEL","Offset Filter Position", "%f",0., 90., 0., 0., 0, 0, 0}, };

 static INumberVectorProperty ofwNPR = {  mydev, "OFFSET_FWHEEL", "offset filter goto",  MAIN_GROUP , IP_RW, 0, IPS_IDLE,  ofwNR, NARRAY(ofwNR), "", 0};



// Engineering stuff for each motor

typedef struct _INDIMOTOR
{
	ILightVectorProperty word0LP;
	ILight word0L[16];	

	ILightVectorProperty word1LP;
	ILight word1L[16];

	ISwitchVectorProperty iowordSP;
	ISwitch iowordS[13];//GPIO, fault state and enable line

	ITextVectorProperty nameTP;
	IText nameT[1];
	char nameString[30];

	ISwitchVectorProperty engSwitchesSP;
	ISwitch engSwithcesS[1];

	int motor_num; //can address


} INDIMOTOR;

INDIMOTOR indi_motors[7];

static IText rawCmdT[] = {{"RAWCMD", "Raw Command", "Response Here", 0, 0, 0}};
static ITextVectorProperty rawCmdTP = {mydev, "RAWCMD", "Raw Command", ENG_GROUP, IP_RW, 0, IPS_IDLE, rawCmdT, NARRAY(rawCmdT) };
char rawCmdString[50];

 /* Note that we must define ISNewBLOB and ISSnoopDevice even if we don't use them, otherwise, the driver will NOT compile */
 void ISNewBLOB (const char *dev, const char *name, int sizes[], int blobsizes[], char *blobs[], char *formats[], char *names[], int n) {}
 void ISSnoopDevice (XMLEle *root) {}



/*############################################################################
#  Title: ISGetProperties
#  Author: C.Johnson
#  Date: 9/9/19
#  Args:  
#  Returns: N/A
#  Description: publishes all properties that will be displayed by the driver
#
#############################################################################*/
 void ISGetProperties (const char *dev)
 {
         if (dev && strcmp (mydev, dev))
             return;
 
/********Telemetry***********/
        IDDefSwitch (&connectSP, NULL);
        IDDefSwitch (&engSwitchSP, NULL);

		IDDefSwitch (&comTypeSP, NULL);

		IDDefText(&networkTP, NULL);
		networkTP.tp[0].text = gnetwork;

		IDDefText(&ttyPortTP, NULL);
		ttyPortTP.tp[0].text = gttyPORT;

		IDDefText(&rawCmdTP, NULL);
		rawCmdT[0].text = rawCmdString;

		fillMotors();

		//TODO this should be read from a config file.
		strcpy(ttyPortTP.tp[0].text, "/dev/ttyUSB0");
		strcpy(networkTP.tp[0].text, "10.130.133.24:10001");
		/*
		IDDefText(&motorStatusTP, NULL);
		motorStatusTP.tp[0].text = gstatusString[0];
		motorStatusTP.tp[1].text = gstatusString[1];
		motorStatusTP.tp[2].text = gstatusString[2];
		motorStatusTP.tp[3].text = gstatusString[3];
		motorStatusTP.tp[4].text = gstatusString[4];
		motorStatusTP.tp[5].text = gstatusString[5];
		motorStatusTP.tp[6].text = gstatusString[6];

		IDDefText(&motor1TP, NULL);
		motor1T[0].text  = gstatusString[0];

		IDDefLight( &motor1W0LP, NULL );


		IDDefText(&motor2TP, NULL);
		motor2T[0].text  = gstatusString[1];

		IDDefLight( &motor2W0LP, NULL );
		*/
//	IDDefText  (&stdTelemTP, NULL);
        
/***********GOTO*************/
	IDDefNumber  (&ufwNPR, NULL);
	IDDefNumber  (&lfwNPR, NULL);
	IDDefNumber  (&offxNPR, NULL);
	IDDefNumber  (&offyNPR, NULL);
	IDDefNumber  (&offFocNPR, NULL);
	IDDefNumber  (&offMirrNPR, NULL);
	IDDefNumber  (&ofwNPR, NULL);
	
	IDDefSwitch  (&actionSP, NULL);
	
	if (!inited)
	{
		//guiderInit();
		//sp->s = states[0];
		//connectS[0].s = ISS_ON;
		//connectS[1].s = ISS_OFF;
		//connectDome();
		IDSetSwitch (&connectSP, "Guider is connected.");
	}
         
 }

/*############################################################################
#  Title: ISNewText
#  Author: C.Johnson
#  Date: 9/9/19
#  Args: default indi definition
#  Returns: N/A
#  Description: Catches all new text events.  Required to be here even if
#	not used 
#
#############################################################################*/
 void ISNewText (const char *dev, const char *name, char *texts[], char *names[], int n)
 {
	IDMessage(mydev, "ISNewText called [%s], %li", texts[0], sizeof(texts[0]));
	char septext[30];
	char respbuff[100];

	if( !strcmp(name, "COM") )
	{
		
		/*After trying for hours this is the only way 
		I cant seem to update a TP 
		IUUpdate, IUSaveText, strcpy, strncpy 
		so instead we point text member to texts[0]
		and update a global variable like a chump.
		*/
		IText *tp = IUFindText( &ttyPortTP, names[0] );
		strcpy(gttyPORT, texts[0]);
		
		strcpy(gttyPORT, texts[0]);
		tp->text = gttyPORT;

		IDSetText(&ttyPortTP, "Changing port to [%s]", ttyPortT[0].text);
	}
	if( !strcmp(name, "RAWCMD") )
	{
		IDMessage(mydev, "SENDING COMMAND %s", texts[0]);
		if(connectSP.s == IPS_OK)
		{
			//TODO: we might want to make this part of the 
			//gb_commands if we are trying to stay away from
			//directo moog_* class.
			moog_write( RS485_FD, texts[0] );
			usleep(2000);
			moog_read( RS485_FD, respbuff );
			strcpy(rawCmdT[0].text, respbuff );
			rawCmdT[0].text[strlen(respbuff)] = '\0';
			IDSetText(&rawCmdTP, NULL);
		}
	}
 	return;
 }

/*############################################################################
#  Title: ISNewNumber
#  Author: C.Johnson
#  Date: 9/9/19
#  Args: default indi definition
#  Returns: N/A
#  Description: Catches all new number events.  Required to be here even if
#	not used 
#
#############################################################################*/
 void ISNewNumber (const char *dev, const char *name, double values[], char *names[], int n)
 { int err; char ret[100];
          
         /* ignore if not ours */
         if (strcmp (dev, mydev))
             return;
 

		
	
	 	 

	if (!strcmp (name, offFocNPR.name)) {
             /* new Focus Position */
             /* Check connectSP, if it is idle, then return */
             if (connectSP.s == IPS_IDLE)
             {
				 
                 offFocNPR.s = IPS_IDLE;
                 IDSetNumber(&offFocNPR, "Guider is offline.");
                 return;
             }
	     
             stageGoTo(RS485_FD, offFocNPR.name, (int)values[0]);
	
	     offFocNPR.s = IPS_IDLE;
	     IDSetNumber(&offFocNPR, NULL);
             return;
	    }
	else if (!strcmp (name, offxNPR.name)) {
             /* new X Position */
             /* Check connectSP, if it is idle, then return */
             if (connectSP.s == IPS_IDLE)
             {
                 offxNPR.s = IPS_IDLE;
                 IDSetNumber(&offxNPR, "Guider is offline.");
                 return;
             }
             
             stageGoTo(RS485_FD, offxNPR.name, (int)values[0]);
	
	     offxNPR.s = IPS_IDLE;
	     IDSetNumber(&offxNPR, NULL);
             return;
	    }
	else if (!strcmp (name, offyNPR.name)) {
             /* new Y Position */
             /* Check connectSP, if it is idle, then return */
             if (connectSP.s == IPS_IDLE)
             {
                 offyNPR.s = IPS_IDLE;
                 IDSetNumber(&offyNPR, "Guider is offline.");
                 return;
             }
             
             stageGoTo(RS485_FD, offyNPR.name, (int)values[0]);
	
	     offyNPR.s = IPS_IDLE;
	     IDSetNumber(&offyNPR, NULL);
             return;
	    }
	else if (!strcmp (name, ufwNPR.name)) {
             /* new upper filter position */
             /* Check connectSP, if it is idle, then return */
             if (connectSP.s == IPS_IDLE)
             {
                 ufwNPR.s = IPS_IDLE;
                 IDSetNumber(&ufwNPR, "Guider is offline.");
                 return;
             }
             
             stageGoTo(RS485_FD, ufwNPR.name, (int)values[0]);
	
	     ufwNPR.s = IPS_IDLE;
	     IDSetNumber(&ufwNPR, NULL);
             return;
	    }
	else if (!strcmp (name, lfwNPR.name)) {
             /* new lower filter position */
             /* Check connectSP, if it is idle, then return */
             if (connectSP.s == IPS_IDLE)
             {
                 lfwNPR.s = IPS_IDLE;
                 IDSetNumber(&lfwNPR, "Guider is offline.");
                 return;
             }
             
             stageGoTo(RS485_FD, lfwNPR.name, (int)values[0]);
	
	     lfwNPR.s = IPS_IDLE;
	     IDSetNumber(&lfwNPR, NULL);
             return;
	    }
	else if (!strcmp (name, ofwNPR.name)) {
             /* new offset filter position */
             /* Check connectSP, if it is idle, then return */
             if (connectSP.s == IPS_IDLE)
             {
                 ofwNPR.s = IPS_IDLE;
                 IDSetNumber(&ofwNPR, "Guider is offline.");
                 return;
             }
             
             stageGoTo(RS485_FD, ofwNPR.name, (int)values[0]);
	
	     ofwNPR.s = IPS_IDLE;
	     IDSetNumber(&ofwNPR, NULL);
             return;
	    }
	else if (!strcmp (name, offMirrNPR.name)) {
             /* new offset mirror position */
             /* Check connectSP, if it is idle, then return */
             if (connectSP.s == IPS_IDLE)
             {
                 offMirrNPR.s = IPS_IDLE;
                 IDSetNumber(&offMirrNPR, "Guider is offline.");
                 return;
             }
             
             stageGoTo(RS485_FD, offMirrNPR.name, (int)values[0]);

	     offMirrNPR.s = IPS_IDLE;
	     IDSetNumber(&offMirrNPR, NULL);
             return;
	    }
	else{
             /* unknown value*/
             
	    }
	
	
 }

/*############################################################################
#  Title: ISNewSwitch
#  Author: C.Johnson
#  Date: 9/9/19
#  Args: default indi definition
#  Returns: N/A
#  Description: Catches all new switch events.  Required to be here even if
#	not used 
#
#############################################################################*/
void ISNewSwitch (const char *dev, const char *name, ISState *states, char *names[], int n)
{
	ISwitch *sp;
 	int i, err;
	char ret[20]; 
	ISState state;	
	int isDifferent;
	char respbuffer[50];

	/* ignore if not ours */
	if (strcmp (dev, mydev))
		return;
	
       if (!strcmp (name, comTypeSP.name)) {
             /* change comm type */
             /* Check comTypeSP, if it is idle, then return */
             
	     
             return;
	    }
	
	if (!strcmp(name, connectSP.name))
	{
		sp = IUFindSwitch (&connectSP, names[0]);
		IDMessage(mydev, "CONNECT HAS BEEN CALLED");
		if (! strcmp(sp->name, connectS[0].name))
		{
			if(states[0] == ISS_ON)
			{
				//if(connectS[0].s == ISS_ON){return;}//hack so code wont run twice
				IUResetSwitch(&connectSP);
				//sp->s = states[0];
				connectS[0].s = ISS_ON;
				//connectS[1].s = ISS_OFF;
				//RS485_FD = ttyOpen(ttyPortT[0].text);
				//RS485_FD = net_ttyOpen("10.130.133.24:10001");
				IDMessage(mydev, "networkT[0].text=%s", networkT[0].text);
				RS485_FD = net_ttyOpen(networkT[0].text);
				if (RS485_FD > 0)
				{
					connectS[0].s = ISS_ON;
					//connectS[1].s = ISS_OFF;
					connectSP.s = IPS_OK;
					IDSetSwitch (&connectSP, "Guider is connected.");
					if(actionS[0].s == ISS_OFF)
					{
						gbIndiInit(  );
						actionS[0].s = ISS_ON;
						IDSetSwitch(&actionSP, "Initializing guider.");
					}
				}
				else
				{
					connectS[0].s = ISS_OFF;
					//connectS[1].s = ISS_ON;
					connectSP.s = IPS_ALERT;
					IDSetSwitch (&connectSP, "Could Not connect to guider on port %s", ttyPortT[0].text);
					inited=0;
				}
			}
			else
			{
				ttyClose(RS485_FD);
				connectS[0].s = ISS_OFF;
				connectSP.s = IPS_IDLE;
				IDSetSwitch(&connectSP, "Disconnecting Guider");
			}		


		}
		else
		{
			//if(connectS[0].s == ISS_OFF){return;}//hack so code wont run twice
			IUResetSwitch(&connectSP);
			//sp->s = states[1];
			connectS[0].s = ISS_OFF;
			connectS[1].s = ISS_ON;
			connectSP.s = IPS_IDLE;
			ttyClose(RS485_FD);
			IDSetSwitch (&connectSP, "Guider is disconnected.");
				
		}
	}

         /* actions */
	else if (! strcmp(name, actionSP.name))
	{
		for (i = 0; i < n; i++) 
		{
			
			/* Find switches with the passed names in the initSP property */
			sp = IUFindSwitch (&actionSP, names[i]);
			state = states[i];
			isDifferent = state != sp->s;
			if(!isDifferent)// no need to update. 
				continue;
			/*  init  */ 
			if (sp == &actionS[0]) 
			{
				if (state == ISS_ON)
				{
					IDMessage(mydev, "Initializing Guider");
					gbIndiInit(  ); 
				}
				else
				{

				}
			}
			
			 
			/*  home  */ 
			else if (sp == &actionS[1]) 
			{
				stageHome(RS485_FD, NULL);
				IDMessage(mydev, "Homing Guider");
				  
			}
			 
			/*  auto  */ 
						 
			IUResetSwitch(&actionSP);
	 
		} /* end for */
	}
	else
	{
		for ( INDIMOTOR *imotor=indi_motors; imotor!=indi_motors+7; imotor++ )
		{
			if( !strcmp(name, imotor->engSwitchesSP.name) )
			{
				moog_callsub(RS485_FD, 110, imotor->motor_num);
				moog_read(RS485_FD, respbuffer);
				IDMessage(mydev, "Moving %s out of limit resp is %s.", imotor->nameT[0].text, respbuffer  );
			}
		}

	}

	
	
}

/*############################################################################
#  Title: guiderInit
#  Author: C.Johnson
#  Date: ???
#  Args:  N/A
#  Description: 
#
#############################################################################*/
 static void gbIndiInit()
 {
	char buff[40];
        // int inited=0;              /* set once mountInit is called */
	guider_init(RS485_FD);
	int init_struct =1;
	
	 //configure driver to talk with tcs
	 //configure();
	
	//sprintf(NET_ADDR, "%s", DOM_NET_ADDR);
	//PORT = DOM_PORT;
	//sprintf(SYSID, "%s", DOM_SYSID);
	
	//IDMessage(mydev, "Connecting To  %s:%i using %s / %s",NET_ADDR,PORT,TELID,SYSID);
	
         zeroTelem(); // zero telem
         
         if (inited)//do not do below functions if already inited
             return;

	/* start timer to simulate mount motion
            The timer will call function mountSim after POLLMS milliseconds */
         IEAddTimer (POLLMS, guiderProc, &init_struct);
	
         inited = 1;
         
 }

/*############################################################################
#  Title: zeroTelem
#  Author: Chris Johnson
#  Date: 11/20/12
#  Args:  N/A
#  Description: Stuffs all of the telemetry strings with Zero
#
#############################################################################*/
static void zeroTelem()
 {
/*        char zerAll[] = "-00.000 00 00 -000.00000 -000.0000000 -000.000000 -000.00000 -000.0000000 -000.0000000 -000.00000 -000.0000000 -000.0000000 -000.00000 00 00";
	 
	
	int  err;
	double num;

        sscanf(zerAll, "%s %s %s %s %s %s %s %s %s %s %s %s %s %s %s", 
		stdTelem.del,stdTelem.mod,stdTelem.init,stdTelem.telaz,
		stdTelem.az,stdTelem.home,stdTelem.cpd,stdTelem.sd,
		stdTelem.w,stdTelem.sdw,stdTelem.nu,stdTelem.rho,stdTelem.phi,
		stdTelem.look,stdTelem.hold);

	sprintf(stdTelem.mod, "STOWED");

	IDSetText(&stdTelemTP, NULL);*/
	
	

 }


/*############################################################################
#  Title: motor2nvp
#  Author: Scott Swindell
#  Date: 9/20/19
#  Args:  motor-> pointer to motor status struct to match with the name
#  Description: 
#		Return the NVP of the corresponding motor name.
#############################################################################*/

static INumberVectorProperty *motor2nvp(MSTATUS *motor)
{
	INumberVectorProperty *NVP;
	if(strcmp( motor->name,  "OFFSET_X") == 0)
	{
		NVP = &offxNPR;
	}	
	else if(strcmp( motor->name,  "OFFSET_Y") == 0)
	{
		NVP = &offyNPR;
	}	
	else if(strcmp( motor->name,  "OFFSET_FOCUS") == 0)
	{
		NVP = &offFocNPR;
	}
	else if(strcmp( motor->name,  "OFFSET_MIRRORS") == 0)
	{
		NVP = &offMirrNPR;
	}
	else if(strcmp( motor->name,  "OFFSET_FWHEEL") == 0)
	{
		NVP = &ofwNPR;
	}
	else if(strcmp( motor->name,  "FWHEEL_UPPER") == 0)
	{
		NVP = &ufwNPR;
	}
	else if(strcmp( motor->name,  "FWHEEL_LOWER") == 0)
	{
		NVP = &lfwNPR;
	}
	else
	{
		NVP = NULL;
	}
	
	return NVP;

}

/*############################################################################
#  Title: guiderTelem
#  Author: Chris Johnson
#  Date: 11/20/12
#  Args:  N/A
#  Description: Retrieves all of the basic telemetry from the TCS and stores
#               it locally for display and later use. The meat of this is done
#				by populating the allmotors array with the doTelemetry call 
#				and putting that information in the indi_motors array. 
#				The indi_motors array much of the motor specific indi properties. 
#
#############################################################################*/
static int guiderTelem(int init_struct)
 {
        char ret[121], ret2[121], guiderResponse[300], mname[30] ;
		
	int  err, ix, isFilter, active;
	double num;
	static MSTATUS allmotors[7];
	INumber *indinum;
	IText * tp;
	MSTATUS *mstat;
	INumberVectorProperty *pNVP;
	ILight *statusLight;
	ILightVectorProperty *statusLightVP;

	ISwitch *ioBitSwitch;
	ISwitchVectorProperty *ioBitSwitchVector;

	char readBuffer[40];
	moog_write( RS485_FD, "RW(13)" );
	moog_read(RS485_FD, readBuffer );
	

	if(init_struct)
	{
		memset(allmotors,0,(sizeof(MSTATUS)*7));
	}

	//TODO: we need some more error checking on doTelemetry. 
	//I occasionally get a situation where we are happily conneced but the 
	//allmotors struct doesn't get initialized properly. 
	//this is caught later when we do the if pNVP == NULL 
	//a few lines below
	
	fprintf(stderr, "init_struct is %i\n", init_struct);
	active = doTelemetry(RS485_FD, allmotors, init_struct);

	//Iterate through the allmotors array so we can populate 
	//indi fields.
	for(MSTATUS *motor=allmotors; motor!=allmotors+7; motor++)
	{
		if(init_struct)
		{
			//apply names to indi_motors from allmotors array.
			//Here we associate the indi_motor index with the 
			//motor->motor_num
			if (indi_motors[motor->motor_num-1].nameT[0].text !=  NULL)
				strncpy(indi_motors[motor->motor_num-1].nameT[0].text,  motor->name, 30);
			indi_motors[motor->motor_num-1].motor_num  = motor->motor_num;
			IDSetText(&indi_motors[motor->motor_num-1].nameTP, NULL);
		}

		//motor2nvp uses the motor->name to 
		//grab the correct number vector property
		//where we will display the position or 
		//filternumber of each axis. Populating this 
		//is 99% of what the user should see and use.
		//The rest of the stuff is for engineering purposes
		
		pNVP = motor2nvp(motor);
		if(pNVP == NULL)
		{//stuct did not init properly.
			IDMessage(mydev, "Bad motor!");
			continue;
		}
		if ( motor->isActive )
		{/*motor->isActive tells us weather the head node was able to communicate with 
		 motor over the can bus.*/
			
			if(motor->isHomed)
				pNVP->s = IPS_OK;
			else
				pNVP->s = IPS_IDLE;
			
			pNVP->np[0].value = motor->pos;
			statusLight = indi_motors[motor->motor_num-1].word0L;
			statusLightVP = &indi_motors[motor->motor_num-1].word0LP;
			ioBitSwitch = indi_motors[motor->motor_num-1].iowordS;
			ioBitSwitchVector = &indi_motors[motor->motor_num-1].iowordSP;
			if(statusLight != NULL)
			{
				for(int sbit=0; sbit<16; sbit++)
				{	
					//IDMessage(mydev, "MOTOR 2 your up, checking bit %i %i %i",sbit, motor->words[0] & (1<<sbit), motor->words[0] );
					if(motor->words[0] & (1<<sbit))
					{
						//IDMessage(mydev, "%s %s %i bit is %s", statusLightVP->name, motor->name, motor->words[0], statusLight[sbit].name);
						statusLight[sbit].s = IPS_ALERT;
					}
					else
					{
						statusLight[sbit].s = IPS_IDLE;
					}
					IDSetLight(statusLightVP, NULL);

					if(motor->iobits & (1<<sbit))
					{
						ioBitSwitch[sbit].s = ISS_ON;
					}
					else
					{
						ioBitSwitch[sbit].s = ISS_OFF;
					}
					IDSetSwitch(ioBitSwitchVector, NULL );

				}
			}


		}
		else
			pNVP->s = IPS_ALERT;

		//snprintf(mname, 30, "M%i", motor->motor_num);

		
		IDSetNumber(pNVP, NULL);
	}


	fprintf(stderr, "in guiderTelem %s\n", allmotors[5].name );
 	return 1;
        

 }



/*############################################################################
#  Title: guiderProc
#  Author: E.C. Downey (Hacked by C. Johnson)
#  Date: 11/20/12
#  Args:  N/A
#  Description: 
#
#############################################################################*/
void guiderProc (void *p)
 {
         static struct timeval ltv;
         struct timeval tv;
         double dt, da, dx, num;
         int nlocked;
		 int init_struct;

    if(p != NULL)// we only need to init the allmotors struct once.
	{
		IDMessage(mydev, "INITING STRUCT");
		init_struct=1;
	}
	else
		init_struct=0;

	fprintf(stderr, "in guider proc\n");
	/* If telescope is not on, do not query.  just start */
         if (connectSP.s == IPS_IDLE)
         {
                 IEAddTimer (POLLMS, guiderProc, NULL);
                 return;
         }
 
	  /* Process per current state.*/
         switch (connectSP.s)
         {
         
         /* #1 State is idle*/
         case IPS_IDLE:
             break;
 
         case IPS_BUSY:
             break;
 
         case IPS_OK:
            /********* TELEMETRY HOOK ********/
		fprintf(stderr, "going to telem\n");
		
	     guiderTelem(init_struct);
            /*********************************/
             break;
 
         case IPS_ALERT:
	     //testConnect();
             break;
         }
 
         /* again */
         IEAddTimer (POLLMS, guiderProc, NULL);
 }
 



/***********************************************************
 *Name: buildMStatusString
 *args: motor -> MSTATUS struct from allmotors array
 *		mstring -> string to populate with motor status infor
 *Description:
 * 				Build a string of usefull status info.
 * 				We no longer use this function to populate
 * 				a string. We decided a set of various widgets
 * 				would be more apt to display status. 
 *		-Scott Swindell 9/2019
 * **********************************************************/
static void buildMStatusString(MSTATUS *motor, char mstring[])
{
	char dummy[100];
	snprintf(dummy, 50, "%s(%i)\n", motor->name, motor->motor_num);
	strcat(mstring, dummy );
	
	if(motor->isActive)
	{
		snprintf(dummy, 30, "Encoder Pos:%i ", motor->pos);
		strcat(mstring, dummy);
		if(motor->isFilter)
			snprintf(dummy, 20, "| Filter Number:%i", motor->fnum);
		strcat(mstring, dummy);
		strcat(mstring, "\n");

	}
	else
	{
		strcat(mstring, "Not Active!");
	}
	
	
}



static void fillMotors()
{
	char name[20];
	char label[20];
	char group[20];
	char code[300];
	int motor_num = 1;
	fprintf(stderr, "FILLING THE MOTORS\n");
	int lenlimit = 30;
	const char * STATUS_CODES[2][16] = {
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
		}
	};
	
	for(INDIMOTOR *imotor=indi_motors; imotor!=indi_motors+7; imotor++)
	{

		
		snprintf( group, 20, "Motor %i Eng", motor_num );
		
		snprintf( name, 20, "M%iIO", motor_num );
		IUFillSwitchVector( &imotor->iowordSP, imotor->iowordS, NARRAY(imotor->iowordS), mydev, name, "IO", group, IP_RO, ISR_NOFMANY, 0, IPS_IDLE);

		snprintf( name, 20, "M%iLIM", motor_num );
		IUFillSwitchVector(&imotor->engSwitchesSP, imotor->engSwithcesS, NARRAY(imotor->engSwithcesS), mydev, name, "Special", group, IP_RW, ISR_NOFMANY, 0, IPS_IDLE);
		IUFillSwitch(imotor->engSwithcesS, name, "Get Out of Limit", IPS_IDLE);
		IDDefSwitch(&imotor->engSwitchesSP, NULL);

		//Name of the motor
		snprintf( name, 20, "M%iName", motor_num );
		IUFillTextVector(&imotor->nameTP, imotor->nameT, NARRAY(imotor->nameT), mydev, name, "Motor Name", group, IP_RO, 0, IPS_IDLE);

		IUFillText(imotor->nameT, "MNAME", "Motor Name", "NULL");

		imotor->nameT[0].text = imotor->nameString;
		IDDefText(&imotor->nameTP, NULL);

		//Motor status words
		snprintf( label, 20, "Motor %i Word 0", motor_num );
		snprintf( name, 20, "M%iW0", motor_num );
		IUFillLightVector(&imotor->word0LP, imotor->word0L, NARRAY(imotor->word0L), mydev, (const char *) name, (const char *) label, group, IPS_IDLE );
		
		

		for(int code_num=0; code_num<16; code_num++)
		{
			snprintf(name, 20, "BIT%i", code_num);
			strncpy(code, STATUS_CODES[0][code_num], lenlimit);
			if( strlen(STATUS_CODES[0][code_num]) > lenlimit )
				strcpy(code+lenlimit, "...");
			code[strlen(code)] = '\0';

			IUFillLight(imotor->word0L+code_num, name, (const char *) code, IPS_IDLE);

			if ( code_num < 11)
			{//only 10 lines of gpio...
				snprintf(name, 20, "IO_%02i", code_num);
				IUFillSwitch(imotor->iowordS+code_num, name, name, ISS_OFF);
			}
			else if(code_num == 11)
			{//... and a line for Not Faulted...
				IUFillSwitch(imotor->iowordS+code_num, "NOFAULT", "Not Fauted", ISS_OFF);
			}
			else if(code_num == 12)
			{//... and a line for Drive enable output which should always be high.
				IUFillSwitch(imotor->iowordS+code_num, "ENBL", "Enabled", ISS_OFF);
				IDMessage(mydev, "WE MADE IT TO ENABLE");
			}
		}
		IDDefLight(&imotor->word0LP, NULL);
		IDDefSwitch(&imotor->iowordSP, NULL);

		snprintf(label, 20, "Motor %i Word 1", motor_num );
		snprintf(name, 20, "M%iW1", motor_num);
		IUFillLightVector(&imotor->word1LP, imotor->word1L, NARRAY(imotor->word1L), mydev, (const char *) name, (const char *) label, group, IPS_IDLE );

		for(int code_num=0; code_num<16; code_num++)
		{//TODO this could be done in the above for loop.
			snprintf(name, 20, "BIT_02%i", code_num);
			strncpy(code, STATUS_CODES[1][code_num], lenlimit);

			if(sizeof(STATUS_CODES[1][code_num]) > lenlimit)
				strcpy(code+lenlimit, "...");
			IUFillLight(imotor->word1L+code_num, name, code, IPS_IDLE);
		}
		IDDefLight(&imotor->word1LP, NULL);

		motor_num++;
	}
}
