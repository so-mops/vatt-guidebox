
#define OFFSET_X 1 //Offset guider x stage
#define OFFSET_Y 2 //Offset guider y stage
#define OFFSET_FOCUS 3 //Offset guider focus stage
#define OFFSET_MIRRORS 4
#define OFFSET_FWHEEL 5
#define FWHEEL_LOWER 6
#define FWHEEL_UPPER 7


#define SENDSIZE 100
#define READSIZE 10000

typedef struct 
{
	int words[4];
	int userbits;
	int motor_num;//CAN bus address
	int pos;
	char name[20];
	int fnum;
	int isActive;
	int isFilter;
	int isHomed;


} MSTATUS;


/* function prototypes */
int open_port( char usbport[] );
int close_port( int fd );
int moog_write( int rs485_fd, const char *msg );
int moog_read( int rs485_fd, char resp[] );
int moog_init(int rs485_fd );
int moog_home( int rs485_fd, int can_addr );
int moog_lgoto(int rs485_fd, int can_addr, int pos );
int moog_fgoto( int rs485_fd, int can_addr, int fnum );
int moog_getstatus(int rs485_fd, MSTATUS* stat);
int moog_getallstatus(int rs485_fd, MSTATUS* stat);
void print_status(MSTATUS stat);
int build_stat_structs( int rs485_fd, MSTATUS motors[] );

