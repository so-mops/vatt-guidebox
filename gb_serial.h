
#define OFFSET_X 1 //Offset guider x stage
#define OFFSET_Y 2 //Offset guider y stage
#define OFFSET_FOCUS 3 //Offset guider focus stage
#define OFFSET_MIRRORS 4
#define OFFSET_FWHEEL 5
#define FWHEEL_LOWER 6
#define FWHEEL_UPPER 7


#define SENDSIZE 100
#define READSIZE 1000

typedef struct 
{
	int words[4]; //status words
	int userbits;
	int iobits;
	int motor_num;//CAN bus address
	int pos;
	char name[20];
	int fnum; //Filter position
	int isActive;
	int isFilter;
	int isHomed;
	int inNegLimit;
	int inPosLimit;
	int isMoving;
	int head_node;
	int fdist; // distance in counts b/w filters
	int neg_slimit;// negative software limits
	int pos_slimit;//positive software limit


} MSTATUS;


/* function prototypes */
int open_port( char usbport[] );
int open_port_net( char *, short );
int close_port( int fd );
int moog_write( int rs485_fd, const char *msg );
int moog_read( int rs485_fd, char resp[] );
int moog_init(int rs485_fd );
int moog_home( int rs485_fd, int can_addr );
int moog_lgoto(int rs485_fd, int can_addr, int pos );
int moog_fgoto( int rs485_fd, int can_addr, int fnum );
int moog_getstatus(int rs485_fd, MSTATUS* stat);
int moog_getallstatus(int rs485_fd, MSTATUS* stat);
int moog_getallstatus_quick(int rs485_fd, MSTATUS* stat);
int moog_callsub( int rs485_fd, int subnum, int can_addr );
void print_status(MSTATUS stat);
int build_stat_structs( int rs485_fd, MSTATUS motors[] );
void gb_lantronix_reset(char *host, short port);


