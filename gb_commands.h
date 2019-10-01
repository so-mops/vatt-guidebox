#include "gb_serial.h"

// Globals

// Function protptypes 
int stageGoTo(int port_fd, char *strAxis, int POSITION);
int guider_init(  int port_fd );
int guiderRead(  int port_fd, char * resp );
int validateAxis(char *axis, int *isFilter);
int doTelemetry(int port_fd, MSTATUS *allmotors, int init_struct);
int stageHome(int ttyfd, char *strAxis);
int ttyOpen(char *ttyPort);
int net_ttyOpen(char *netinfo);
void ttyClose(int ttyfd);

