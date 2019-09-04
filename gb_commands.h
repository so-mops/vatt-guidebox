#include "gb_serial.h"

// Globals
int RS485_FD;
MSTATUS allmotors[7];

// Function protptypes 
void offsetXGoTo (int RS485_FD, int POSITION);
void offsetYGoTo (int RS485_FD, int POSITION);
void offsetFocusGoTo (int RS485_FD, int POSITION);
void offsetMirrorsGoTo (int RS485_FD, int POSITION);
void offsetFilterGoTo (int RS485_FD, int POSITION);
void lowerFilterGoTo (int RS485_FD, int POSITION);
void upperFilterGoTo (int RS485_FD, int POSITION);
int guider_init( int act, char *usbport );

