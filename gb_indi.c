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
#include <math.h>


/* header for this project */
#include "gb_commands.h"
	


 /* Definitions */

#define TTYPORT "/dev/ttyUSB0"

#define mydev		"INDI-VATT-GUIDEBOX"
#define MAIN_GROUP	"Guider Control"                  /* Group name */
#define ENG_GROUP	"Engineering"
#define qrydev	 	"FILTERS"
#define logdev	 	"LOGGER"

#define MOT1_GROUP "MOTOR 1 Eng"
#define MOT2_GROUP "MOTOR 2 Eng"
#define MOT3_GROUP "MOTOR 3 Eng"
#define MOT4_GROUP "MOTOR 4 Eng"
#define MOT5_GROUP "MOTOR 5 Eng"
#define MOT6_GROUP "MOTOR 6 Eng"
#define MOT7_GROUP "MOTOR 7 Eng"


//TODO these should be in a config file
#define ENCODER2MM (1.0/2000.0)
#define XOFFSET 102.6
#define YOFFSET 39.5
#define YOFFSET_UMIRROR 58.25

#define POLLMS          1000                             /* poll period, ms */

#define NET 0
#define SER 1

#define OFF 0
#define ON 1

typedef enum{
	XTRANS,
	YTRANS,
	FOCUSTRANS
} TRANS;

int RS485_FD;
int inited;
int polling=0;




static void gbIndiInit();
static void zeroTelem();
void guiderProc (void *p);
static void buildMStatusString(MSTATUS *, char *);
static void fillMotors();
static void fillFWheels();
static int saveFNames(char *, char *, char *);
static int loadFNames(char *, char[5][20]);
static int getCurrentFilters(char *, char *);
static void defMotors();
static void defWheels();
static void printAllmotors();
double focus_trans(double, double);

//global memory for IText properties
char gttyPORT[20];
char gnetwork[20];
char gstatusString[NMOTORS][1000];
int gcomtype;

// main connection switch
static ISwitch connectS[] = {
	//   {"CONNECT",  "Connect",  ISS_OFF, 0, 0}
     {"CONNECT",  "Connect",  ISS_OFF, 0, 0}, {"DISCONNECT", "Disconnect", ISS_ON, 0, 0}
	 };
static ISwitchVectorProperty connectSP = { mydev, "CONNECTION", "Connection",  MAIN_GROUP, IP_RW, ISR_1OFMANY, 0, IPS_IDLE,  connectS, NARRAY(connectS), "", 0 };
 
static ISwitch getdataS[] = {
	{"GETDATA",  "Get Data",  ISS_OFF, 0, 0}
     //{"GETDATA",  "On",  ISS_OFF, 0, 0}, {"DISGETDATA", "Off", ISS_ON, 0, 0}
	 };
static ISwitchVectorProperty getdataSP = { qrydev, "GETDATA", "Get Data",  MAIN_GROUP, IP_RW, ISR_NOFMANY, 0, IPS_IDLE,  getdataS, NARRAY(getdataS), "", 0 };

		

// connection type switch
static ISwitch comTypeS[] = {
	 {"NETWORK",  "Network",  ISS_OFF, 0, 0},
         {"SERIAL",  "Serial",  ISS_ON, 0, 0}
	 };
static ISwitchVectorProperty comTypeSP = { mydev, "COMTYPE", "Connection Type",  ENG_GROUP, IP_RW, ISR_1OFMANY, 0, IPS_IDLE,  comTypeS, NARRAY(comTypeS), "", 0 };

static ISwitch resetLtxS[] = {{"RESET_LTX",  "Reset Lantronix",  ISS_OFF, 0, 0},};
static ISwitchVectorProperty resetLtxSP = { mydev, "lTXRESET", "Lantronix",  ENG_GROUP, IP_RW, ISR_NOFMANY, 0, IPS_IDLE,  resetLtxS, NARRAY(resetLtxS), "", 0 };

static ISwitch serialfixS[] = {{"FIXSERIAL",  "Fix Serial Line",  ISS_OFF, 0, 0},};
static ISwitchVectorProperty serialfixSP = { mydev, "SFIX", "FIX",  ENG_GROUP, IP_RW, ISR_NOFMANY, 0, IPS_IDLE,  serialfixS, NARRAY(serialfixS), "", 0 };



static IText networkT[] =  {{"NETWORK", "Net", "10.0.3.15:4900", 0, 0, 0}};
static ITextVectorProperty networkTP = {  mydev, "NET", "Guider Network Information",  ENG_GROUP , IP_RW, 0, IPS_IDLE,  networkT, NARRAY(networkT), "", 0};

static IText ttyPortT[] =  {{"TTYport", "TTY Port", "/dev/ttyUSB0", 0, 0, 0}};
static ITextVectorProperty ttyPortTP = {  mydev, "COM", "Guider Communication",  ENG_GROUP , IP_RW, 0, IPS_IDLE,  ttyPortT, NARRAY(ttyPortT), "", 0};



static ISwitch engSwitchsS[] = {{"SLIMITS", "FIND SOFT LIMITS", ISS_OFF, NULL, 0}};
static ISwitchVectorProperty engSwitchSP = {mydev, "Switches", "Switches", ENG_GROUP, IP_RW, ISR_NOFMANY, 0, IPS_IDLE, engSwitchsS, NARRAY(engSwitchsS), "", 0};

/************************************************
Group:  MAIN
************************************************/
//guider actions
static ISwitch actionS[]  = {{"INITIALIZE",  "Initialize",  ISS_OFF, 0, 0},{"HOME",  "Reference",  ISS_OFF, 0, 0}};

ISwitchVectorProperty actionSP      = { mydev, "GUIDE_BOX_ACTIONS", "Guide Box Actions",  MAIN_GROUP, IP_RW, ISR_NOFMANY, 0, IPS_IDLE,  actionS, NARRAY(actionS), "", 0 };



//Upper Filter Goto
static INumber ufwNR[] = {{"FWHEEL_UPPER", "Upper Filter", "%f",0., 0., 0., 0., 0, 0, 0}, };

 static INumberVectorProperty ufwNPR = {  mydev, "FWHEEL_UPPER", "Upper Filter Wheel goto",  MAIN_GROUP , IP_RW, 0, IPS_IDLE,  ufwNR, NARRAY(ufwNR), "", 0};

//Lower Filter Goto
static INumber lfwNR[] = {{"FWHEEL_LOWER","Lower Filter", "%f",0., 0., 0., 0., 0, 0, 0}, };

 static INumberVectorProperty lfwNPR = {  mydev, "FWHEEL_LOWER", "Lower Filter Wheel goto",  MAIN_GROUP , IP_RW, 0, IPS_IDLE,  lfwNR, NARRAY(lfwNR), "", 0};

//Offset X Goto
static INumber offxNR[] = {{"OFFSET_X","Offset X", "%5.2f",0., 0., 0., 0., 0, 0, 0}, };

 static INumberVectorProperty offxNPR = {  mydev, "OFFSET_X", "offset x goto",  MAIN_GROUP , IP_RW, 0, IPS_IDLE,  offxNR, NARRAY(offxNR), "", 0};

//Offset Y Goto
static INumber offyNR[] = {{"OFFSET_Y","Offset Y", "%5.2f",0., 0., 0., 0., 0, 0, 0}, };

 static INumberVectorProperty offyNPR = {  mydev, "OFFSET_Y", "offset y goto",  MAIN_GROUP , IP_RW, 0, IPS_IDLE,  offyNR, NARRAY(offyNR), "", 0};



//Offset Focus Goto
//
/*
 * An unfortunate naming scheme...
 * The Focus in "OFFSET_FOCUS" refers to the offset guider. This is how we 
 * refer to the imager guider as opposed to the slit view (spectrogrpha) guider. 
 * The offFocNR widget is built to change the focus motor position of the offset 
 * guider. For reasons I won't get into, the position input from the user
 * is separated into two parts an input position (z) and a user offset (z'). This offset
 * is completely different from the "OFFSET" term used to differentiate the 
 * guiders. So, enjoy that future programmers!
 *
 * The positon that will be sent to the OFFSET_FOCUS motor will be
 * p=z+f(x,y)+z'
 * Where f is the focus_trans function that subtracts the substantial 
 * curvature of the field and is a function of the x and y positions
 * of the offset guider. 
 *
 * */
static INumber offFocInputNR[] = {{"USER_POS","Focus", "%5.2f",0., 0., 0., 0., 0, 0, 0},
				{"USER_OFFSET","User Offset", "%5.2f",0., 0., 0., 0., 0, 0, 0}};

static INumberVectorProperty offFocInputNPR = {  mydev, "FOCUS_INPUT", "Offset Focus GOTO",  MAIN_GROUP , IP_WO, 0, IPS_IDLE,  offFocInputNR, NARRAY(offFocInputNR), "", 0};

static INumber offFocNR[] = {{"OFFSET_FOCUS","Focus", "%5.2f",0., 0., 0., 0., 0, 0, 0}};

static INumberVectorProperty offFocNPR = {  mydev, "OFFSET_FOCUS", "Focus Pos.",  MAIN_GROUP , IP_RO, 0, IPS_IDLE,  offFocNR, NARRAY(offFocNR), "", 0};

//Offset Mirror Goto
static INumber offMirrNR[] = {{"OFFSET_MIRRORS","Offset Mirror Position", "%5.2f",0., 90., 0., 0., 0, 0, 0}, };

static INumberVectorProperty offMirrNPR = {  mydev, "OFFSET_MIRRORS", "offset mirror goto",  MAIN_GROUP , IP_RW, 0, IPS_IDLE,  offMirrNR, NARRAY(offMirrNR), "", 0};

//Offset Filter Goto
static INumber ofwNR[] = {{"OFFSET_FWHEEL","Offset Filter Position", "%5.2f",0., 90., 0., 0., 0, 0, 0}, };

static INumberVectorProperty ofwNPR = {  mydev, "OFFSET_FWHEEL", "offset filter goto",  MAIN_GROUP , IP_RW, 0, IPS_IDLE,  ofwNR, NARRAY(ofwNR), "", 0};



/*****************************************************
 * 		A note on Text Properties in INDI:
 *		Each IText struct has a char pointer
 *		member named "text". The memory this pointer
 *		points to needs to be allocated. I do this 
 *		in this module by creating a global char 
 *		array like head_nodeString. We might be able 
 *		to do this with a malloc but the memory 
 *		needs to globally accessible  because all 
 *		our VectorProperties are global. Doing this 
 *		statically with a char arrays seems like the way to go. 
 *		As far as I can tell, this causes IUUpdateText to 
 *		flip out and give a seg fault. Instead of IUUpdate, 
 *		I use strcpy to copy whatever text I want to the 
 *		IText.text memeber and update the client with IDSetText. 
 *************************************************************/


// Head Node Name
static IText head_nodeT[] = {{"HEAD", "Head Motor Node", "IDK?", 0, 0, 0}};
static char head_nodeString[20];
static ITextVectorProperty head_nodeTP = { mydev, "HEAD", "Head Motor Node", ENG_GROUP , IP_RO, 0, IPS_IDLE,  head_nodeT, NARRAY(head_nodeT), "", 0};

//Auto Focus Button
static ISwitch autoFocusS[1] = {{"AUTOFOC", "Auto Focus", ISS_OFF, 0, 0}};
static ISwitchVectorProperty autoFocusSP = {mydev, "AUTOFOC", "", MAIN_GROUP, IP_RW, ISR_NOFMANY, 0, IPS_IDLE, autoFocusS, NARRAY(autoFocusS), "", 0};

//Filter wheel buttons
static ISwitch lfS[5];
static ISwitchVectorProperty lfSP;

static ISwitch ufS[5];
static ISwitchVectorProperty ufSP;

static ISwitch gfS[]  = {
	{"GF0S",  "Clear",  ISS_OFF, 0, 0},
	{"GF1S",  "Bl+ND",  ISS_OFF, 0, 0},
	{"GF2S",  "Blue",  ISS_OFF, 0, 0},
	{"GF3S",  "Rd+ND",  ISS_OFF, 0, 0},
	{"GF4S",  "Red",  ISS_OFF, 0, 0},
	};

ISwitchVectorProperty gfSP      = { mydev, "GFS", "Guider Wheel",  MAIN_GROUP, IP_RW, ISR_1OFMANY, 0, IPS_IDLE,  gfS, NARRAY(gfS), "", 0 };


//Umirror/Centerfield position
static ISwitch mirr_posS[]  = {
	{"CENTER",  "Center",  ISS_OFF, 0, 0},
	{"UMIRROR",  "U-Mirror",  ISS_OFF, 0, 0},
	{"CALIB",  "Calib.",  ISS_OFF, 0, 0},
	};

ISwitchVectorProperty mirr_posSP      = { mydev, "MIRRORS", "Mirror",  MAIN_GROUP, IP_RW, ISR_1OFMANY, 0, IPS_IDLE,  mirr_posS, NARRAY(mirr_posS), "", 0 };


// User given names for filters in lower filter wheel
static IText lfnT[] = {
	{"F0", "Filter 0", "Clear", 0, 0, 0},
	{"F1", "Filter 1", "lf1", 0, 0, 0},
	{"F2", "Filter 2", "lf2", 0, 0, 0},
	{"F3", "Filter 3", "lf3", 0, 0, 0},
	{"F4", "Filter 4", "lf4", 0, 0, 0},
};

char lfnChars[5][30];

static ITextVectorProperty lfnTP = { mydev, "LOWER_FNAMES", "Lower Filter Wheel Names", MAIN_GROUP , IP_RW, 0, IPS_IDLE,  lfnT, NARRAY(lfnT), "", 0};


// User given names for filters in upper filter wheel
static IText ufnT[] = {
	{"F0", "Filter 0", "Clear", 0, 0, 0},
	{"F1", "Fiuter 1", "uf1", 0, 0, 0},
	{"F2", "Fiuter 2", "uf2", 0, 0, 0},
	{"F3", "Fiuter 3", "uf3", 0, 0, 0},
	{"F4", "Fiuter 4", "uf4", 0, 0, 0},
};

char ufnChars[5][30];

static ITextVectorProperty ufnTP = { mydev, "UPPER_FNAMES", "Upper Filter Wheel Names", MAIN_GROUP , IP_RW, 0, IPS_IDLE,  ufnT, NARRAY(ufnT), "", 0};


// User given names for filters in upper filter wheel
static IText gfnT[] = {
	{"F0", "Filter 0", "U", 0, 0, 0},
	{"F1", "Filter 1", "B", 0, 0, 0},
	{"F2", "Filter 2", "V", 0, 0, 0},
	{"F3", "Filter 3", "R", 0, 0, 0},
	{"F4", "Filter 4", "I", 0, 0, 0},
};

char gfnChars[5][30];

static ITextVectorProperty gfnTP = { mydev, "GUIDER_FNAMES", "Guider Filter Wheel Names", MAIN_GROUP , IP_RW, 0, IPS_IDLE,  gfnT, NARRAY(gfnT), "", 0};


// Engineering stuff for each motor

typedef struct _INDIMOTOR
{
	ILightVectorProperty word0LP;
	ILight word0L[16];

	ILightVectorProperty word1LP;
	ILight word1L[16];

	ILightVectorProperty userbitsLP;
	ILight userbitsL[16];
	

	ISwitchVectorProperty iowordSP;
	ISwitch iowordS[13];//GPIO, fault state and enable line

	ITextVectorProperty nameTP;
	IText nameT[1];
	char nameString[30];

	ISwitchVectorProperty engSwitchesSP;
	ISwitch engSwitchesS[2];

	INumberVectorProperty softLimitsNP;
	INumber softLimitsN[2];

	INumberVectorProperty fdistNP;
	INumber fdsitN[1];

	int motor_num; //CAN address

} INDIMOTOR;

//The structures in this array are filled
//in the fillmotors function.

INDIMOTOR indi_motors[NMOTORS];

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
	static int firstGetProperties = 1;
	char filter_names[5][20];
	char upper[20], lower[20];
	
	if (dev && strcmp (mydev, dev))
	{
		if(!strcmp(qrydev, dev))
		{
			getCurrentFilters(upper, lower);
			IDMessage(qrydev, "upper:%s lower:%s", upper, lower);
		}
		return;
	}

	if(firstGetProperties)
	{
		networkTP.tp[0].text = gnetwork;
		ttyPortTP.tp[0].text = gttyPORT;
		rawCmdT[0].text = rawCmdString;
		head_nodeT[0].text = head_nodeString;
		strcpy(ttyPortTP.tp[0].text, "/dev/ttyUSB0");
		strcpy(networkTP.tp[0].text, "10.0.3.15:4900");
		fillFWheels();		
		fillMotors();
		firstGetProperties=0;
	}

	IDDefSwitch (&connectSP, NULL);
	IDDefSwitch (&engSwitchSP, NULL);

	IDDefSwitch (&comTypeSP, NULL);

	IDDefSwitch (&resetLtxSP, NULL);

	IDDefSwitch(&getdataSP, NULL);
	IDDefText(&networkTP, NULL);

	IDDefText(&ttyPortTP, NULL);

	IDDefText(&rawCmdTP, NULL);
	
	IDDefSwitch(&lfSP, NULL);
IDDefSwitch(&ufSP, NULL);
	IDDefSwitch(&gfSP, NULL);

	IDDefSwitch(&serialfixSP, NULL);
	IDDefSwitch(&mirr_posSP, NULL);

	IDDefSwitch(&autoFocusSP, NULL);

	IDDefText(&head_nodeTP, NULL);

	defMotors();
	defWheels();


	gcomtype = SER;
        
	/***********GOTO*************/
	IDDefNumber  (&ufwNPR, NULL);
	IDDefNumber  (&lfwNPR, NULL);
	IDDefNumber  (&offxNPR, NULL);
	IDDefNumber  (&offyNPR, NULL);
	IDDefNumber  (&offFocNPR, NULL);
	IDDefNumber  (&offFocInputNPR, NULL);
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

	char septext[30];
	char respbuff[100];

	if( strcmp(mydev, dev) )
	{
		return;
	}
	if( !strcmp(name, "COM") )
	{
		
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
			//
			IDMessage(mydev, "WRITING...");
			moog_write( RS485_FD, texts[0] );
			usleep(50000);
			IDMessage(mydev, "READING...");
			moog_read(RS485_FD, respbuff);
			strcpy(rawCmdT[0].text, respbuff );
			rawCmdT[0].text[strlen(respbuff)] = '\0';
			IDMessage(mydev, "Are we here %s", respbuff);
			IDSetText(&rawCmdTP, "Response is %s", respbuff);
		}
	}
	if( !strcmp(name, "LOWER_FNAMES") )
	{
		
		//IUUpdateText(&lfnTP, texts, names, n);
		for(int jj=0; jj<n; jj++)
		{
			if( strcmp(texts[jj],  lfnT[jj].text) )
			{
				strcpy( lfnT[jj].text, texts[jj] );
				saveFNames(lfnTP.name, texts[jj], lfnT[jj].name );
			}
		}
		IDSetText(&lfnTP, NULL );
		
	}	
	
	if( !strcmp(name, "UPPER_FNAMES") )
	{
		
		//IUUpdateText(&ufnTP, texts, names, n);
		for(int jj=0; jj<n; jj++)
		{
			if( strcmp(texts[jj], ufnT[jj].text) != 0 )
			{
				strcpy( ufnT[jj].text, texts[jj] );
				saveFNames(ufnTP.name, texts[jj], ufnT[jj].name );
			}
		}
		IDSetText(&ufnTP, NULL );
	}

	if( !strcmp(name, "GUIDER_FNAMES") )
	{
		
		for(int jj=0; jj<n; jj++)
		{
			strcpy( gfnT[jj].text, texts[jj] );
		}
		IDSetText(&gfnTP, NULL );
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
 { 
	int err; char ret[100];
	double zmm, zprimemm, xmm, ymm, focusmm, delta_focusmm, focus_curve_subtraction;
         /* ignore if not ours */
	if (strcmp (dev, mydev))
		return;
 


	if (!strcmp (name, offFocInputNPR.name)) {
             /* new Focus Position */
             /* Check connectSP, if it is idle, then return */
             if (connectSP.s == IPS_IDLE)
             {
				 
                 offFocInputNPR.s = IPS_IDLE;
                 IDSetNumber(&offFocNPR, "Guider is offline.");
                 return;
             }
	     zmm=0;
	     zprimemm=0;
	     for(int ii=0; ii<n; ii++ )
	     {
	     	 if( !strcmp(names[ii], "USER_POS") )
		 	zmm=values[ii];
		 else if( !strcmp(names[ii], "USER_OFFSET")  )
			 zprimemm = values[ii];
	     }
             xmm = offxNPR.np[0].value;
             ymm = offyNPR.np[0].value;
	     offFocInputNPR.np[0].value=zmm;
	     offFocInputNPR.np[1].value=zprimemm;
	     focus_curve_subtraction = focus_trans(xmm, ymm);

	     if(autoFocusS[0].s == ISS_ON)
	     {
             	stageGoTo(RS485_FD, offFocNPR.name, (int)((zmm + zprimemm + focus_curve_subtraction)/ENCODER2MM));
	     }
	     else
	     {
	     	stageGoTo(RS485_FD, offFocNPR.name, (int)((zmm + zprimemm)/ENCODER2MM));
	     }
	
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

             xmm = offxNPR.np[0].value;
             ymm = offyNPR.np[0].value;
	     zmm = offFocInputNPR.np[0].value;
	     zprimemm = offFocInputNPR.np[1].value;
	     delta_focusmm = focus_trans(values[0], ymm);
	     focus_curve_subtraction = focus_trans(values[0], ymm);

	     if(autoFocusS[0].s == ISS_ON)
	     {
             	stageGoTo(RS485_FD, offFocNPR.name, (int)((zmm + zprimemm + focus_curve_subtraction)/ENCODER2MM));
	     }
             stageGoTo(RS485_FD, offxNPR.name, (int)((values[0]+XOFFSET)/ENCODER2MM));
	
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
 
	     xmm = offxNPR.np[0].value;
             ymm = offyNPR.np[0].value;
	     zmm = offFocInputNPR.np[0].value;
	     zprimemm = offFocInputNPR.np[1].value;
	     focus_curve_subtraction = focus_trans(xmm, values[0]);
	     if(autoFocusS[0].s == ISS_ON)
	     {
             	stageGoTo(RS485_FD, offFocNPR.name, (int)((zmm + zprimemm + focus_curve_subtraction)/ENCODER2MM));
	     }

             //TODO check which mirros is in.
             stageGoTo(RS485_FD, offyNPR.name, (int)((values[0]+YOFFSET)/ENCODER2MM));
	
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
             
             stageGoTo(RS485_FD, offMirrNPR.name, (int)(values[0]/ENCODER2MM));

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
char L, F, S;
int fnum, init_struct=1;
double focus_curve_subtraction, xmm, ymm, zmm, zprimemm;

	/* ignore if not ours */
	if (strcmp (dev, mydev))
		return;
	else if (! strcmp(name, comTypeSP.name))
		{
		IUUpdateSwitch(&comTypeSP, states, names, n);
		comTypeSP.s = IPS_BUSY;
		if (comTypeS[0].s == ISS_ON)
			{
			gcomtype = NET; 
			IDMessage(mydev, "Changing COM to Network");
		
			}
		else if (comTypeS[1].s == ISS_ON)
			{
			gcomtype = SER;
			IDMessage(mydev, "Changing COM to Serial");
			}
		comTypeSP.s = 0;
		IDSetSwitch(&comTypeSP, NULL);
			
            return;
		
	}

	else if (! strcmp(name, resetLtxSP.name))
		{
		IUUpdateSwitch(&resetLtxSP, states, names, n);
		resetLtxSP.s = IPS_BUSY;
		if (resetLtxS[0].s == ISS_ON)
            		{
			resetLtxS[0].s = ISS_OFF;
                	IDSetSwitch(&resetLtxSP, "lantronix is resetting.");
			lantronix_reset(networkT[0].text);
			resetLtxSP.s = 0;
	    		IDSetSwitch(&resetLtxSP, "Lantronix Reset Complete");
            		}
            	else
                	IDSetSwitch(&resetLtxSP, "false trigger.");
	    
		return;
		 
				 
		}
	
	else if (!strcmp(name, connectSP.name))
	{
		sp = IUFindSwitch (&connectSP, names[0]);
		if (! strcmp(sp->name, connectS[0].name))
		{
			if(states[0] == ISS_ON)
			{
				//if(connectS[0].s == ISS_ON){return;}//hack so code wont run twice
				IUResetSwitch(&connectSP);
				//sp->s = states[0];
				connectS[0].s = ISS_ON;
				//connectS[1].s = ISS_OFF;
				if (gcomtype == NET)
				{
					RS485_FD = net_ttyOpen(networkT[0].text);
					IDMessage(mydev, "networkT[0].text=%s", networkT[0].text);
				}
				else
				{
					RS485_FD = ttyOpen(ttyPortT[0].text);
				}

				if (RS485_FD > 0)
				{
					connectS[0].s = ISS_ON;
					connectSP.s = IPS_OK;
					IDSetSwitch (&connectSP, NULL);
					moog_write(RS485_FD, "Rxxx");

					if( moog_read(RS485_FD, ret ) == 0 )
					{//check if the head node has been initialized.
						if(atoi(ret) != MOOG_INITIALIZED)
						{// if not initialize. This will loose the home position.

							IDMessage(mydev, "Guidebox is not initialized.");
							IDMessage(mydev, "Initializing.");
							gbIndiInit( );
						}
						else
						{
							IDMessage(mydev, "Guidebox is initialized.");
						}

					}
					else
					{
						IDMessage(mydev, "Guidebox did not respond to init status check.");
						IDMessage(mydev, "Initializing.");
						gbIndiInit( );
					}
						

					//if the motors don't respond try initializing.
					

         				if(!polling)
					{
						IEAddTimer (POLLMS, guiderProc, &init_struct);
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


				ufwNPR.s = IPS_IDLE;
				IDSetNumber(&ufwNPR, "DID THIS HAPPEN??");

				lfwNPR.s = IPS_IDLE;
				IDSetNumber(&lfwNPR, NULL);

				offxNPR.s = IPS_IDLE;
				IDSetNumber(&offxNPR, NULL);

				offyNPR.s = IPS_IDLE;
				IDSetNumber(&offyNPR, NULL);

				offFocNPR.s = IPS_IDLE;
				IDSetNumber(&offFocNPR, NULL);

				offMirrNPR.s = IPS_IDLE;
				IDSetNumber(&offMirrNPR, NULL);

				ofwNPR.s = IPS_IDLE;
				IDSetNumber(&ofwNPR, NULL);

				lfSP.s = IPS_IDLE;
				IDSetSwitch(&lfSP, NULL);

				ufSP.s = IPS_IDLE;
				IDSetSwitch(&ufSP, NULL);

				mirr_posSP.s = IPS_IDLE;
				IDSetSwitch(&mirr_posSP, NULL);

				gfSP.s = IPS_IDLE;
				IDSetSwitch(&gfSP, NULL);

				actionSP.s = IPS_IDLE;
				IDSetSwitch(&actionSP, NULL);

			}		


		}
		else
		{
			
			//if(connectS[0].s == ISS_OFF){return;}//hack so code wont run twice
			if(states[0] == ISS_ON)
			{
				ttyClose(RS485_FD);
				connectSP.s = IPS_IDLE;
				IDSetSwitch(&connectSP, "Disconnecting Guider");


				ufwNPR.s = IPS_IDLE;
				IDSetNumber(&ufwNPR, NULL);

				lfwNPR.s = IPS_IDLE;
				IDSetNumber(&lfwNPR, NULL);

				offxNPR.s = IPS_IDLE;
				IDSetNumber(&offxNPR, NULL);

				offyNPR.s = IPS_IDLE;
				IDSetNumber(&offyNPR, NULL);

				offFocNPR.s = IPS_IDLE;
				IDSetNumber(&offFocNPR, NULL);

				offMirrNPR.s = IPS_IDLE;
				IDSetNumber(&offMirrNPR, NULL);

				ofwNPR.s = IPS_IDLE;
				IDSetNumber(&ofwNPR, NULL);

				lfSP.s = IPS_IDLE;
				IDSetSwitch(&lfSP, NULL);

				ufSP.s = IPS_IDLE;
				IDSetSwitch(&ufSP, NULL);

				mirr_posSP.s = IPS_IDLE;
				IDSetSwitch(&mirr_posSP, NULL);

				gfSP.s = IPS_IDLE;
				IDSetSwitch(&gfSP, NULL);

				actionSP.s = IPS_IDLE;
				IDSetSwitch(&actionSP, NULL);


			}
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
	else if( !strcmp(name, lfSP.name) )
	{
		
		if(connectS[0].s == ISS_OFF)
		{
			IUResetSwitch(&lfSP);

			IDSetSwitch(&lfSP, "Not connected");
			return;
		}
		for(int ii=0; ii<n; ii++)
		{
			if(states[ii] == ISS_ON );
			{
				int wc = sscanf(names[ii], "%*C%*C%i%*C", &fnum);
				if(wc == 4)
				{
					IDMessage(mydev, "Could not match filter %s", names[ii] );
				}
				else
				{

					IDMessage( mydev, "Match for %i", fnum );
					stageGoTo( RS485_FD, lfwNPR.name, fnum );
				}

			}
		}
	}	
	else if( !strcmp(name, ufSP.name) )
	{	
		
		if(connectS[0].s == ISS_OFF)
		{
			IUResetSwitch(&ufSP);
			IDSetSwitch(&ufSP, "Not connected");
			return;
		}

		for(int ii=0; ii<n; ii++)
		{
			if(states[ii] == ISS_ON );
			{
				int wc = sscanf(names[ii], "%*C%*C%i%*C", &fnum);
				if(wc == 4)
				{
					
					IDMessage(mydev, "Could not match filter %s", names[ii] );

				}
				else
				{

					IDMessage(mydev, "Match for %i", fnum );
					stageGoTo( RS485_FD, ufwNPR.name, fnum );
				}

			}
		}
	}

	else if( !strcmp(name, gfSP.name) )
	{	
		
		if(connectS[0].s == ISS_OFF)
		{
			IUResetSwitch(&gfSP);
			IDSetSwitch(&gfSP, "Not connected");
			return;
		}

		for(int ii=0; ii<n; ii++)
		{
			if(states[ii] == ISS_ON );
			{
				int wc = sscanf(names[ii], "%*C%*C%i%*C", &fnum);
				if(wc == 4)
				{
					
					IDMessage(mydev, "Could not match filter %s", names[ii] );

				}
				else
				{

					IDMessage(mydev, "Match for %i", fnum );
					stageGoTo( RS485_FD, ofwNPR.name, fnum );
				}

			}
		}
	}
	else if( !strcmp(name, mirr_posSP.name))
	{
		for( int ii=0; ii<n; ii++)
		{
			if(states[ii] == ISS_ON)
			{
				//TODO the 4 in the callsub fxns
				//should not be hard coded. It is
				//the address of the OFFSET_MIRRORS
				//stage. Motor->num names should 
				// come from the allmotors array
				// or the indi counterpart.
				if(!strcmp(names[ii], "CENTER"))
				{	
					moog_callsub(RS485_FD, 502, 4);
					break;
				}
				else if(!strcmp(names[ii], "UMIRROR"))
				{
					moog_callsub(RS485_FD, 501, 4);
					break;
				}
				else if(!strcmp(names[ii], "CALIB"))
				{
					moog_callsub(RS485_FD, 503, 4);
					break;
				}

			}
		}
	}
	else if( !strcmp(name, serialfixSP.name))
	{
		moog_serialfix(RS485_FD);
		IUResetSwitch(&serialfixSP);
		IDSetSwitch(&serialfixSP, "Sent Serial Fix");
	}

	else if( !strcmp(name, autoFocusSP.name))
	{//Auto focus routine.
		if(connectS[0].s == ISS_OFF)
		{//we aren't connected.
			autoFocusS[0].s = ISS_OFF;
			IDSetSwitch(&autoFocusSP, NULL);
			return;
		}
		
             	xmm = offxNPR.np[0].value;
		ymm = offyNPR.np[0].value;
		zmm = offFocInputNPR.np[0].value;
		zprimemm = offFocInputNPR.np[1].value;
		focus_curve_subtraction = focus_trans(xmm, ymm);
             	stageGoTo(RS485_FD, offFocNPR.name, (zmm+zprimemm+focus_curve_subtraction)/ENCODER2MM );
		if(states[0] == ISS_ON)
		{
			autoFocusSP.s=IPS_OK;
			autoFocusS[0].s = ISS_ON;
             		stageGoTo(RS485_FD, offFocNPR.name, (zmm+zprimemm+focus_curve_subtraction)/ENCODER2MM );
		}
		else
		{
			autoFocusSP.s=IPS_IDLE;
			autoFocusS[0].s = ISS_OFF;
             		stageGoTo(RS485_FD, offFocNPR.name, (zmm+zprimemm)/(ENCODER2MM) );
		}
		IDSetSwitch(&autoFocusSP, NULL);

	}
	else
	{// This is the motor specific engineering tools
		for ( INDIMOTOR *imotor=indi_motors; imotor!=indi_motors+NMOTORS; imotor++ )
		{
			if( !strcmp(name, imotor->engSwitchesSP.name) )
			{
				for(int ii=0; ii<n; ii++)
				{
					if( !strcmp( names[ii], imotor->engSwitchesS[0].name ) )
					{//Get out of limit
						moog_callsub(RS485_FD, 110, imotor->motor_num);
					}
					else if( !strcmp( names[ii], imotor->engSwitchesS[1].name ) )
					{
						moog_callsub(RS485_FD, 104, imotor->motor_num);
					}
				}
				imotor->engSwitchesS[0].s = ISS_OFF;
				imotor->engSwitchesS[1].s = ISS_OFF;
				IDSetSwitch(&imotor->engSwitchesSP, NULL );
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
	
         
         if (inited)//do not do below functions if already inited
             return;

	/* start timer to simulate mount motion
            The timer will call function mountSim after POLLMS milliseconds */
	
         inited = 1;
         
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
	char fwheel_upper[5], fwheel_lower[5];
	char lower_name[20];
	int fwheel_upper_isOff, fwheel_lower_isOff;
	int  err, ix, isFilter, active, allHomed=1;
	double num;
	static MSTATUS allmotors[NMOTORS];
	static int solenoid_status=OFF;
	static int last_solenoid_status=OFF;
	INumber *indinum;
	IText * tp;
	MSTATUS *mstat;
	INumberVectorProperty *pNVP;
	ILight *w0_statusLight;
	ILightVectorProperty *w0_statusLightVP;	

	ILight *w1_statusLight;
	ILightVectorProperty *w1_statusLightVP;	

	//not implented yet
	ILight *w2_statusLight;
	ILightVectorProperty *w2_statusLightVP;	
	
	ILight *userbitsLights;
	ILightVectorProperty *userBitsVP;
	//end not implemented 
	int faulted=0;

	int iter=1;

	ISwitch *ioBitSwitch;
	ISwitchVectorProperty *ioBitSwitchVector;

	char readBuffer[40];
	moog_write( RS485_FD, "RW(13)" );
	moog_read(RS485_FD, readBuffer );
	int head_node;
	if(init_struct)
	{
		memset(allmotors,0,(sizeof(MSTATUS)*NMOTORS));
	}

	//TODO: we need some more error checking on doTelemetry. 
	//I occasionally get a situation where we are happily conneced but the 
	//allmotors struct doesn't get initialized properly. 
	//this is caught later when we do the if pNVP == NULL 
	//a few lines below
	
	active = doTelemetry( RS485_FD, allmotors, init_struct );

	if(init_struct)
	{
		head_node = allmotors[0].head_node;
	}

	//Iterate through the allmotors array so we can populate 
	//indi fields.
	for(MSTATUS *motor=allmotors; motor!=allmotors+NMOTORS; motor++)
	{
		
		if(init_struct)
		{
			//apply names to indi_motors from allmotors array.
			//Here we associate the indi_motor index with the 
			//motor->motor_num
			if ( indi_motors[motor->motor_num-1].nameT[0].text !=  NULL )
				strncpy( indi_motors[motor->motor_num-1].nameT[0].text,  motor->name, 30 );
			indi_motors[motor->motor_num-1].motor_num  = motor->motor_num;
			//IDSetText( &indi_motors[motor->motor_num-1].nameTP, NULL );
			if( motor->head_node == motor->motor_num )
			{
				strcpy( head_nodeT[0].text, motor->name );
				IDSetText( &head_nodeTP, NULL );
			}
		}

		//motor2nvp uses the motor->name to 
		//grab the correct number vector property
		//where we will display the position or 
		//filternumber of each axis. 	
		
		pNVP = motor2nvp(motor);
		//IDMessage(mydev, "%s %i %i %i", motor->name, motor->words[0], motor->words[1], motor->userbits);
		if(pNVP == NULL)
		{//stuct did not init properly.
			IDMessage(mydev, "%i Motor %s (Num %i) was not intialized correctly in the allmotors array. This indicates a communication failure.", iter, motor->name, motor->motor_num);
			//We should probably bail out and disconnect here.
			continue;
		}

		if(!motor->isHomed)
		{
			//ignore homed status if 
			//motor is not powered
			if(motor->isActive)
				allHomed=0;
		}

		if ( motor->isActive )
		{/*motor->isActive tells us weather the head node was able to communicate with 
		 motor over the can bus.*/

			if ( motor->inNegLimit )
			{
				pNVP->s = IPS_ALERT; 
				IDMessage(mydev, "%s in negative limit!", motor->name);
			}
			else if(motor->inPosLimit)
			{
				pNVP->s = IPS_ALERT; 
				IDMessage(mydev, "%s in positive limit!", motor->name);
			}

			else if(!motor->isHomed)
			{
				pNVP->s = IPS_BUSY; 
			}
			else if(motor->isHomed && !motor->isMoving)
			{
				pNVP->s = IPS_OK; 
			}
			else if(motor->isHomed && motor->isMoving)
			{
				pNVP->s = IPS_BUSY;
			}

			faulted=0;
			if( motor->iobits & (1<<11) )
			{//Check for a motor fault
				pNVP->s = IPS_ALERT; 
				faulted=1;
				IDMessage(mydev, "%s Faulted!", motor->name);
			}

			//pNVP->np[0].value = motor->pos*ENCODER2MM;
			w0_statusLight = indi_motors[motor->motor_num-1].word0L;
			w0_statusLightVP = &indi_motors[motor->motor_num-1].word0LP;
			w1_statusLight = indi_motors[motor->motor_num-1].word1L;
			w1_statusLightVP = &indi_motors[motor->motor_num-1].word1LP;
			ioBitSwitch = indi_motors[motor->motor_num-1].iowordS;
			ioBitSwitchVector = &indi_motors[motor->motor_num-1].iowordSP;

			if(w0_statusLight != NULL)
			{
				for(int sbit=0; sbit<16; sbit++)
				{	
					if(motor->words[0] & (1<<sbit))
					{
						w0_statusLight[sbit].s = IPS_ALERT;
					}
					else
					{
						w0_statusLight[sbit].s = IPS_IDLE;
					}
					if(motor->words[1] & (1<<sbit))
					{
						w1_statusLight[sbit].s = IPS_ALERT;
					}
					else
					{
						w1_statusLight[sbit].s = IPS_IDLE;
					}



					if(motor->iobits & (1<<sbit))
					{
						ioBitSwitch[sbit].s = ISS_ON;
					}
					else
					{
						ioBitSwitch[sbit].s = ISS_OFF;
					}

				}

				//TODO: the SetLight and SetSwitch only need to be called
				//if the io or status words change.
				//THis could help keep communication to 
				//the client minimal. We could keep a 
				//copy of the allmotors array before any
				//changes are made and compare it after 
				//we run through this loop.
				IDSetLight(w0_statusLightVP, NULL);
				IDSetLight(w1_statusLightVP, NULL);
				IDSetSwitch(ioBitSwitchVector, NULL );
			}

		}
		//motor specific things
		if( !strcmp( motor->name, "FWHEEL_LOWER" ) )
		{
			if( motor->isMoving )
			{
				lfSP.s = IPS_BUSY;	
			}
			else if(faulted)
			{
				lfSP.s = IPS_ALERT;
			}
			else
			{
				IUResetSwitch(&lfSP);
				lfS[motor->fnum].s = ISS_ON;
				lfSP.s = IPS_OK;
				for(int ii=1; ii<5; ii++)
				{
					strcpy(lfS[ii].label, lfnT[ii].text);
				}
				if( !motor->isHomed)
				{
					lfSP.s = IPS_BUSY;
				}
						
			}
			IDSetSwitch(&lfSP, NULL);

			fwheel_lower_isOff =motor->words[0] & 2;
		}

		else if( !strcmp( motor->name, "FWHEEL_UPPER" ) )
		{
			if( motor->isMoving )
			{
				ufSP.s = IPS_BUSY;	
			}
			else if(faulted)
			{
				ufSP.s = IPS_ALERT;
			}
			else
			{
				IUResetSwitch(&ufSP);
				ufS[motor->fnum].s = ISS_ON;
				ufSP.s = IPS_OK;	
				for(int ii=1; ii<5; ii++)
				{
					strcpy(ufS[ii].label, ufnT[ii].text);
				}

				if( !motor->isHomed)
				{
					ufSP.s = IPS_BUSY;
				}

			}
			IDSetSwitch(&ufSP, NULL);	
	
			fwheel_upper_isOff = motor->words[0] & 2;

		}
		else if( !strcmp( motor->name, "OFFSET_FWHEEL" ) )
		{
			if( motor->isMoving )
			{
				gfSP.s = IPS_BUSY;	
			}
			else if( faulted )
			{
				gfSP.s = IPS_ALERT;
			}
			else
			{
				IUResetSwitch(&gfSP);
				gfSP.s = IPS_OK;	
				gfS[motor->fnum].s = ISS_ON;	

				if( !motor->isHomed)
				{
					gfSP.s = IPS_BUSY;
				}

			}
			IDSetSwitch(&gfSP, NULL);
		}
		else if( !strcmp( motor->name, "OFFSET_X" ) )
		{
				pNVP->np[0].value = (motor->pos*ENCODER2MM)-XOFFSET;
		}
		else if( !strcmp( motor->name, "OFFSET_Y" ) )
		{
				pNVP->np[0].value = (motor->pos*ENCODER2MM)-YOFFSET;
		}

		else if( !strcmp( motor->name, "OFFSET_FOCUS" ) )
		{
			 pNVP->np[0].value = (motor->pos*ENCODER2MM);
			 if(motor->isHomed)
			 {
				 offFocInputNPR.s = IPS_OK;
			 }
			 else
			 {
				 offFocInputNPR.s = IPS_BUSY;
			 }
			 IDSetNumber(&offFocInputNPR, NULL);
		}
		else if( !strcmp( motor->name, "OFFSET_MIRRORS" ) )
		{
			pNVP->np[0].value = (motor->pos*ENCODER2MM);

			mirr_posS[0].s = ISS_OFF;
			mirr_posS[1].s = ISS_OFF;
			mirr_posS[2].s = ISS_OFF;

			if((166.0 < motor->pos*ENCODER2MM) && (motor->pos*ENCODER2MM < 169.0))
			{
				mirr_posS[0].s = ISS_ON;
			}
			else if(( 2.0 < motor->pos*ENCODER2MM) && (motor->pos*ENCODER2MM < 4.0 ) )
			{
				mirr_posS[1].s = ISS_ON;
			}
			else if(( -1.0 < motor->pos*ENCODER2MM ) && ( motor->pos*ENCODER2MM < 1.0 ) )
			{
				mirr_posS[1].s = ISS_ON;
			}

			if(motor->isMoving)
			{
				mirr_posSP.s = IPS_BUSY;
			}
			else if(!motor->isHomed)
			{
				mirr_posSP.s = IPS_BUSY;
			}
			else
			{
				mirr_posSP.s = IPS_OK;
			}

			if(faulted)
			{
				mirr_posSP.s = IPS_ALERT;
			}
			
			IDSetSwitch(&mirr_posSP, NULL);
				
		}
		iter++;
		IDSetNumber(pNVP, NULL);
	}
	if(allHomed)
	{
		actionSP.s = IPS_OK;
	}
	else
	{
		actionSP.s = IPS_BUSY;
	}
	IDSetSwitch(&actionSP, NULL );

	
	//solenoid sensor stays on due to bug in firmware
	//we shall fix it here until the firmware is fixed. 
	if(fwheel_lower_isOff && fwheel_upper_isOff)
	{
		if( solenoid_status == ON )
		{
			IDMessage(mydev, "FHWEELS On and should be off");
			moog_write(RS485_FD, "OR(7):7");
			moog_write(RS485_FD, "OR(8):7");
		}
		solenoid_status = OFF;
	}
	else
	{
		IDMessage(mydev, "FHWEELS ON");
		solenoid_status = ON;
	}

	//fprintf(stderr, "in guiderTelem %s\n", allmotors[5].name );
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
		init_struct=1;
	}
	else
	{
		init_struct=0;
	}

	//fprintf(stderr, "in guider proc\n");
	/* If guidebox is not on, do not query.  just start */
         if (connectSP.s == IPS_IDLE)
         {
	 	polling=1;
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
		//fprintf(stderr, "going to telem\n");
		
	     guiderTelem(init_struct);
            /*********************************/
             break;
 
         case IPS_ALERT:
	     //testConnect();
             break;
         }
 
         /* again */
	 polling=1;
         IEAddTimer (POLLMS, guiderProc, NULL);
 }
 

/******************************************************
 * Name: fillMotors
 * Description: 
 * 	Populate the indi_motors array of structs
 *	with default values. I beleieve all of this
 *	information is hidden from the average user.
 *	It is mostly used for engineering.
 *	This is mostly done with the IUFill* family 
 *	of functions
 *
 *	Scott Swindell Nov 2019
 *
 *
 * ****************************************************/
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
	
	for(INDIMOTOR *imotor=indi_motors; imotor!=indi_motors+NMOTORS; imotor++)
	{

		snprintf( group, 20, "Motor %i Eng", motor_num );
		
		snprintf( name, 20, "M%iIO", motor_num );
		IUFillSwitchVector( &imotor->iowordSP, imotor->iowordS, NARRAY(imotor->iowordS), mydev, name, "IO", group, IP_RO, ISR_NOFMANY, 0, IPS_IDLE);

		snprintf( name, 20, "M%iLIM", motor_num );
		IUFillSwitchVector(&imotor->engSwitchesSP, imotor->engSwitchesS, NARRAY(imotor->engSwitchesS), mydev, name, "Special", group, IP_RW, ISR_NOFMANY, 0, IPS_IDLE);
		IUFillSwitch(imotor->engSwitchesS, name, "Get Out of Limit", IPS_IDLE);
		
		snprintf( name, 20, "M%iHOME", motor_num );
		IUFillSwitch(imotor->engSwitchesS+1, name, "Home", IPS_IDLE);
		IDDefSwitch(&imotor->engSwitchesSP, NULL);

		snprintf( name, 20, "M%iName", motor_num );
		IUFillTextVector(&imotor->nameTP, imotor->nameT, NARRAY(imotor->nameT), mydev, name, "Motor Name", group, IP_RO, 0, IPS_IDLE);

		IUFillText(imotor->nameT, "MNAME", "Motor Name", "NULL");

		imotor->nameT[0].text = imotor->nameString;
		IDDefText(&imotor->nameTP, NULL);

		snprintf( label, 20, "Motor %i Word 0", motor_num );
		snprintf( name, 20, "M%iW0", motor_num );
		IUFillLightVector(&imotor->word0LP, imotor->word0L, NARRAY(imotor->word0L), mydev, (const char *) name, (const char *) label, group, IPS_IDLE );
		
		//Populate the status bits with the correct code
		for(int code_num=0; code_num<16; code_num++)
		{//word 1
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
				//IDMessage(mydev, "WE MADE IT TO ENABLE");
			}
		}

		IDDefLight(&imotor->word0LP, NULL);
		IDDefSwitch(&imotor->iowordSP, NULL);

		snprintf(label, 20, "Motor %i Word 1", motor_num );
		snprintf(name, 20, "M%iW1", motor_num);
		IUFillLightVector(&imotor->word1LP, imotor->word1L, NARRAY(imotor->word1L), mydev, (const char *) name, (const char *) label, group, IPS_IDLE );

		for(int code_num=0; code_num<16; code_num++)
		{//word 2
			//TODO this could be done in the above for loop.
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


/*****************************************************
 * Name: defMotors
 * Description:
 * 	iterate through the motors and call the IDDef*
 * 	family of functions. This sends the initial
 * 	definition of the INDI vector properties. 
 *
 *
 *	Scott Swindell Nov. 2019
 *
 *
 * ************************************************/
static void defMotors()
{

	for(INDIMOTOR *imotor=indi_motors; imotor!=indi_motors+NMOTORS; imotor++)
	{
		IDDefSwitch(&imotor->engSwitchesSP, NULL);
		//IDDefText(&imotor->nameTP, NULL);
		IDDefLight(&imotor->word0LP, NULL);
		IDDefSwitch(&imotor->iowordSP, NULL);
		IDDefLight(&imotor->word1LP, NULL);
	}
}


/******************************************************
 * Name: fillMotors
 * Description: 
 * 	Populate the indi_motors array of structs
 *	with default values.  
 *	This is done with the IUFill* family 
 *	of functions
 *
 *	Scott Swindell Nov 2019
 *
 *
 * ****************************************************/
static void fillFWheels()
{
		//TODO should probably be in a config file
		char upper_name[20], lower_name[20];
		const char guider_filters[][20] = {
			"U",
			"B",
			"V",
			"R",
			"I"
		};
		const char default_filters[][20] = {
			"Clear",
			"Filter 1",
			"Filter 2",
			"Filter 3",
			"Filter 4"
		};

		char upper_fnames[5][20];
		char lower_fnames[5][20];
		loadFNames(lfnTP.name, lower_fnames);
		loadFNames(ufnTP.name, upper_fnames);

		for(int ii=0; ii<5; ii++)
		{	//point the text propterties at there 
			//assoctiated char arrays
			
			strcpy( lfnChars[ii], lower_fnames[ii] );
			lfnT[ii].text = lfnChars[ii];

			strcpy( ufnChars[ii], upper_fnames[ii] );
			ufnT[ii].text = ufnChars[ii];
			
			sprintf(upper_name, "UF%iS", ii);
			sprintf(lower_name, "LF%iS", ii);
			IUFillSwitch(&ufS[ii], upper_name, upper_fnames[ii], ISS_OFF);
			IUFillSwitch(&lfS[ii], lower_name, lower_fnames[ii], ISS_OFF);

			strcpy( gfnChars[ii], guider_filters[ii] );
			gfnT[ii].text = gfnChars[ii];
		}
		IUFillSwitchVector( &ufSP, ufS, NARRAY(ufS), mydev, "UFS", "Upper Wheel",  MAIN_GROUP, IP_RW, ISR_1OFMANY, 0, IPS_IDLE);
		IUFillSwitchVector( &lfSP, lfS, NARRAY(lfS), mydev, "LFS", "Lower Wheel",  MAIN_GROUP, IP_RW, ISR_1OFMANY, 0, IPS_IDLE);
}


/*****************************************************
 * Name: defWheels
 * Description:
 * 	iterate through the filter wheels and call the IDDef*
 * 	family of functions. This sends the initial
 * 	definition of the INDI vector properties. 
 *
 *
 *	Scott Swindell Nov. 2019
 *
 *
 * ************************************************/

static void defWheels()
{
	for(int ii=0; ii<5; ii++)
	{
		IDDefText(&lfnTP, NULL);
		IDDefText(&ufnTP, NULL);
		IDDefText(&gfnTP, NULL);
	}
}

/**************************************
 *Name focus_trans
 *Description: The focus plane of the guider
 *	Camera is vary curved. This function
 *	adjusts for the curvature of the 
 *	field. The coefficients A, B and C
 *	where determined experimentally 
 *	by a Paul Gabor (pgaboer@as.arizona.edu)
 *	and Daewook Kim ( dkim@optics.arizona.edu)
 *
 *
 *
 * **************************************/
double focus_trans(double x, double y)
{

	double asecs2mm=0.20866666666666667;
	double r = sqrt(x*x + y*y);
	double A=33.325*asecs2mm*asecs2mm;
	double B=0.0063*asecs2mm;
	double C=0.0012;
	return (A*r*r - B*r + C)/1000;
}


static int saveFNames(char *wheel, char *name, char *fnum)
{
	char saved_filters[200];
	char filters[5][20];
	int wc, num;
	FILE *fid;
	sscanf(fnum, "%*c%i", &num );
		
	strcpy(filters[0], "Clear");
	if( access( wheel, F_OK ) != 1 )
		fid = fopen(wheel, "r");
	else	
	{
		fid = NULL;
		IDMessage(mydev, "fid is null fo sure");
	}

	
	if(fid)
	{
		
		wc = fscanf(fid, "%[^,], %[^,], %[^,], %[^,], %s", filters[0], filters[1], filters[2], filters[3], filters[4] );
		fclose(fid);
		fid = fopen(wheel, "w");//clear the file
		if(wc == 5)
		{
			strcpy( filters[num], name );
			IDMessage(mydev, "%s, %s, %s, %s, %s", filters[0], filters[1], filters[2], filters[3], filters[4]);
			fprintf(fid, "%s, %s, %s, %s, %s", filters[0], filters[1], filters[2], filters[3], filters[4]);
		}
		else
		{// oops corrupted file
			for(int ii=0; ii<5; ii++)
			{
				sprintf( filters[ii], "Filter %i", ii );
			}
			
			strcpy( filters[num], name );
			fprintf(fid, "%s, %s, %s, %s, %s", filters[0], filters[1], filters[2], filters[3], filters[4]);
		}	
	}
	else
	{//No wheel file... let's make one
		fid = fopen(wheel, "w");
		for(int ii=0; ii<5; ii++)
		{
			sprintf( filters[ii], "Filter %i", ii  );
		}
		fprintf(stderr, "SHOULD BE SETTING %i %s %s\n", num, fnum, name );
		strcpy( filters[num], name);
		fprintf( fid, "%s, %s, %s, %s, %s", filters[0], filters[1], filters[2], filters[3], filters[4]);
	}
	
	fclose(fid);
}

static int loadFNames(char * wheel, char filters[5][20])
{
	FILE *fid;
	if( access( wheel, F_OK ) != 1 )
		fid = fopen(wheel, "r");
	else
	{
		fid = NULL;
	}
	
	if(fid)
	{
		//comma separated filters.
		int wc = fscanf(fid, "%[^,], %[^,], %[^,], %[^,], %s", filters[0], filters[1], filters[2], filters[3], filters[4] );

		//the first filter is always clear.
		strcpy(filters[0], "Clear");
	}
	else
	{//File doesn't exist
		for(int ii=0; ii<5; ii++)
		{
			if(ii!=0)
				sprintf(filters[ii], "Filter %i", ii);
			else
				sprintf(filters[ii], "Clear");
			
		}
	}
	for(int ii=0; ii<5; ii++)
	{
		fprintf(stderr, "%i %s %s \n", ii, wheel, filters[ii]);
	}
}


static int getCurrentFilters(char *upper, char *lower)
{
	int iter=0, lower_num=0, upper_num=0;
	
	for(ISwitch *lower=lfS; lower!=lfS+5; lower++)
	{	
		if(lower->s == ISS_ON)
		{
			lower_num=iter;
			break;
		}
		iter++;
	}

	iter=0;
	for(ISwitch *upper=ufS; upper!=ufS+5; upper++)
	{
		if(upper->s == ISS_ON)
		{
			upper_num=iter;
			break;
		}
		iter++;
	}

	if( ufnT[upper_num].text != NULL)
		strcpy(upper, ufnT[upper_num].text);
	else
		strcpy(upper, "IDK");
	
	if( lfnT[lower_num].text != NULL)
		strcpy(lower, lfnT[lower_num].text);
	else
		strcpy(lower, "IDK");

}



