#include "gb_serial.h"

// Globals
//int RS485_FD;
MSTATUS allmotors[7];

// Function protptypes 
void offsetXGoTo (int RS485_FD, int POSITION);
void offsetYGoTo (int RS485_FD, int POSITION);
void offsetFocusGoTo (int RS485_FD, int POSITION);
void offsetMirrorsGoTo (int RS485_FD, int POSITION);
void offsetFilterGoTo (int RS485_FD, int POSITION);
void lowerFilterGoTo (int RS485_FD, int POSITION);
void upperFilterGoTo (int RS485_FD, int POSITION);
int guider_init(  int port_fd );
int port_init( int open, char *usbport, int port_fd );
int validateAxis(char *axis, int *isFilter);
