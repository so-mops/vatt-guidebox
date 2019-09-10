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
 #define MAIN_GROUP      "Guider Control"                  /* Group name */
 
  #define POLLMS          250                             /* poll period, ms */

int RS485_FD;

// main connection switch
   static ISwitch connectS[] = {
     {"CONNECT",  "On",  ISS_OFF, 0, 0}, {"DISCONNECT", "Off", ISS_ON, 0, 0}};
 
 static ISwitchVectorProperty connectSP = { mydev, "CONNECTION", "Connection",  MAIN_GROUP, IP_RW, ISR_1OFMANY, 0, IPS_IDLE,  connectS, NARRAY(connectS), "", 0 };

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


		


/************************************************
Group:  MAIN
************************************************/
//guider actions
static ISwitch actionS[]  = {{"INITIALIZE",  "Initialize",  ISS_OFF, 0, 0},{"HOME",  "Home",  ISS_OFF, 0, 0}};

ISwitchVectorProperty actionSP      = { mydev, "GUIDE_BOX_ACTIONS", "Guide Box Actions",  MAIN_GROUP, IP_RW, ISR_NOFMANY, 0, IPS_IDLE,  actionS, NARRAY(actionS), "", 0 };


/*
#define OFFSET_X 1 //Offset guider x stage
#define OFFSET_Y 2 //Offset guider y stage
#define OFFSET_FOCUS 3 //Offset guider focus stage
#define OFFSET_MIRRORS 4
#define OFFSET_FWHEEL 5
#define FWHEEL_LOWER 6
#define FWHEEL_UPPER 7
*/

//Upper Filter Goto
static INumber ufwNR[] = {{"UPPER FILTER","Upper Filter", "%i",0., 90., 0., 0., 0, 0, 0}, };

 static INumberVectorProperty ufwNPR = {  mydev, "FWHEEL_UPPER", "Upper Filter Wheel goto",  MAIN_GROUP , IP_RW, 0, IPS_IDLE,  ufwNR, NARRAY(ufwNR), "", 0};

//Lower Filter Goto
static INumber lfwNR[] = {{"LOWER FILTER","Lower Filter", "%i",0., 90., 0., 0., 0, 0, 0}, };

 static INumberVectorProperty lfwNPR = {  mydev, "FWHEEL_LOWER", "Lower Filter Wheel goto",  MAIN_GROUP , IP_RW, 0, IPS_IDLE,  lfwNR, NARRAY(lfwNR), "", 0};

//Offset X Goto
static INumber offxNR[] = {{"OFFSET X POSITION","Offset X Position", "%i",0., 90., 0., 0., 0, 0, 0}, };

 static INumberVectorProperty offxNPR = {  mydev, "OFFSET_X", "offset x goto",  MAIN_GROUP , IP_RW, 0, IPS_IDLE,  offxNR, NARRAY(offxNR), "", 0};

//Offset Y Goto
static INumber offyNR[] = {{"OFFSET Y POSITION","Offset Y Position", "%i",0., 90., 0., 0., 0, 0, 0}, };

 static INumberVectorProperty offyNPR = {  mydev, "OFFSET_Y", "offset y goto",  MAIN_GROUP , IP_RW, 0, IPS_IDLE,  offyNR, NARRAY(offyNR), "", 0};

//Offset Focus Goto
static INumber offFocNR[] = {{"OFFSET FOCUS","Offset Focus", "%i",0., 90., 0., 0., 0, 0, 0}, };

 static INumberVectorProperty offFocNPR = {  mydev, "OFFSET_FOCUS", "offset foc goto",  MAIN_GROUP , IP_RW, 0, IPS_IDLE,  offFocNR, NARRAY(offFocNR), "", 0};

//Offset Mirror Goto
static INumber offMirrNR[] = {{"OFFSET MIRROR POSITION","Offset Mirror Position", "%i",0., 90., 0., 0., 0, 0, 0}, };

 static INumberVectorProperty offMirrNPR = {  mydev, "OFFSET_MIRRORS", "offset mirror goto",  MAIN_GROUP , IP_RW, 0, IPS_IDLE,  offMirrNR, NARRAY(offMirrNR), "", 0};

//Offset Filter Goto
static INumber ofwNR[] = {{"OFFSET FILTER POSITION","Offset Filter Position", "%i",0., 90., 0., 0., 0, 0, 0}, };

 static INumberVectorProperty ofwNPR = {  mydev, "OFFSET_FWHEEL", "offset filter goto",  MAIN_GROUP , IP_RW, 0, IPS_IDLE,  ofwNR, NARRAY(ofwNR), "", 0};


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
	IDDefText  (&stdTelemTP, NULL);
        
/***********GOTO*************/
	IDDefNumber  (&ufwNPR, NULL);
	IDDefNumber  (&lfwNPR, NULL);
	IDDefNumber  (&offxNPR, NULL);
	IDDefNumber  (&offyNPR, NULL);
	IDDefNumber  (&offFocNPR, NULL);
	IDDefNumber  (&offMirrNPR, NULL);
	IDDefNumber  (&ofwNPR, NULL);
	
	IDDefSwitch  (&actionSP, NULL);
	
/*	if (!inited)
	{
		domeInit();
		//sp->s = states[0];
		connectS[0].s = ISS_ON;
		connectS[1].s = ISS_OFF;
		connectDome();
		IDSetSwitch (&connectSP, "Dome is connected.");
	}*/
         
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

	/* ignore if not ours */
	if (strcmp (dev, mydev))
		return;
	
         
	if (!strcmp(name, connectSP.name))
	{
		sp = IUFindSwitch (&connectSP, names[0]);

		if (! strcmp(sp->name, connectS[0].name))
		{
			if(connectS[0].s == ISS_ON){return;}//hack so code wont run twice
			IUResetSwitch(&connectSP);
			//sp->s = states[0];
			connectS[0].s = ISS_ON;
			connectS[1].s = ISS_OFF;
			connectSP.s = IPS_OK;
			RS485_FD = ttyOpen(TTYPORT);
			IDSetSwitch (&connectSP, "Guider is connected.");

		}
		else
		{
			if(connectS[0].s == ISS_OFF){return;}//hack so code wont run twice
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
				IDMessage(mydev, "Initializing Guider");
				guider_init( RS485_FD ); 
			}
			 
			/*  home  */ 
			else if (sp == &actionS[1]) 
			{
				IDMessage(mydev, "Homing Guider");
				  
			}
			 
			/*  auto  */ 
						 
			IUResetSwitch(&actionSP);
	 
		} /* end for */
	}

	
	
}









