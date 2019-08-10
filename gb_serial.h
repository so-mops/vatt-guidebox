
#define OFFSET_X 1 //Offset guider x stage
#define OFFSET_Y 2 //Offset guider y stage
#define OFFSET_FOCUS 3 //Offset guider focus stage
#define OFFSET_MIRRORS 4
#define OFFSET_FWHEEL 5
#define FWHEEL_LOWER 6
#define FWHEEL_UPPER 7


#define SENDSIZE 100
#define READSIZE 100



typedef struct 
{
	int words[4];
	int userbits;
	int motor_num;//CAN bus address
	char name[20];

} MSTATUS;

const char * STATUS_CODES[4][16] = {
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
	},

	{
		"Error on Communications Channel 0",
		"Error on Communications Channel 1",
		"USB Error (Class 6 only)",
		"Reserved 3",
		"CAN Port Error",
		"Reserved 5",
		"Ethernet Error (Class 6 only)",
		"I2C Running",
		"Watchdog Event",
		"ADB (Animatics Data Block) Bad Checksum",
		"Program Running",
		"Trace in Progress",
		"EE Write Buffer Overflow",
		"EE Busy",
		"Command Syntax Error",
		"Program Checksum Error",
	},

	{
		"Reserved 0",
		"Torque Saturation",
		"Voltage Saturation",
		"Wraparound Occurred",
		"KG Enabled",
		"Velocity Direction",
		"Torque Direction",
		"I/O Fault Latch",
		"Relative Position Mode",
		"Reserved 9",
		"Peak Current Saturation",
		"Modulo Rollover",
		"Brake Asserted",
		"Brake OK",
		"External Go Enabled",
		"Velocity Target Reached",
	}

};

