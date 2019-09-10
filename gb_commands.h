#include "gb_serial.h"

// Globals

// Function protptypes 
int stageGoTo(int port_fd, char *strAxis, int POSITION);
int guider_init(  int port_fd );
int validateAxis(char *axis, int *isFilter);
int doTelemetry(int port_fd, MSTATUS *);
int stageHome(int ttyfd, char *strAxis);
int ttyOpen(char *ttyPort);
void ttyClose(int ttyfd);

