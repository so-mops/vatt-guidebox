/**********************************************
*Title: ng_server.c
*Author: Chris Johnson
*date 10/22/14
*Description: This is a TCP server that handes
*	the NG protocol. server_entry() is the
*	beginning that inits everthing and
*	starts the loop.  all of the important
*	config can be done in the #defines.
*
*
*
**********************************************/

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <unistd.h> /* close */
//#include "weather.h"
#include "ng_server.h"

//general entry point for the server
//void server_entry();


//server parameters !!!!!!!!!!! MODIFY HERE !!!!!!!!!!!!!!!!!
#define SERVER_PORT 5750
#define TELESCOPE_ID "VATT"
#define SYSTEM_ID "GUIDEBOX"

//max string length
#define MAX_MSG 1000

//message identifiers^
#define TELID_ERR       "TELID ERROR"
#define SYSID_ERR       "SYSID ERROR"
#define CMD_REQ "REQUEST"
#define CMD_CMD "COMMAND"
#define CMD_SUCCESS "OK"
#define CMD_REQ_SUCCESS "OK"
#define CMD_ERR "ERROR"
#define CMD_UNKNOWN "UNKNOWN_CMD"
#define CMD_REQ_ERR "REQUEST_ERROR"
#define CMD_CMD_ERR "COMMAND_ERROR"
#define NODATA "NULL"

#define NOK 1
#define OK 0

//data structures
#define MAX_ARGS 100
#define ARG_LEN 40
typedef char ARG_ARRAY[MAX_ARGS][ARG_LEN];

typedef struct
{
char telID[ARG_LEN];
char sysID[ARG_LEN];
unsigned short int pID;
char cmd[ARG_LEN];
ARG_ARRAY cmdArgs;
}TCS_MESSAGE;

void init_data(TCS_MESSAGE *ret_message);
void gen_string(TCS_MESSAGE *ret_message, char *out_string);
void cpy_header(TCS_MESSAGE *new_message, TCS_MESSAGE *ret_message);

int call_daves_command_hook(ARG_ARRAY inArgs);
int call_daves_request_hook(ARG_ARRAY inArgs, ARG_ARRAY outArgs);
int packet_handler(int newSd);
int msg_handler(char *in_string, char *out_string);

/**********************************************
*Title: server_entry
*Author: Chris Johnson
*date 6/22/05
*Description: this is the general entry point
*	for the server loop...  uses TCP/IP
* to listen for packets and processes them. This 
* particilar function only listens and accepts then calls 
* another function..
* Originally this was intended to spawn a thread after every
*accept but I found this to be pointless as I can still 
* hit 100hz in a single thread. 
**********************************************/
void *server_entry(void *data)
{
int sd, newSd, cliLen, iaddr, goodconnect;
unsigned int yes = 1;
struct sockaddr_in cliAddr, servAddr;                  
char in_string[MAX_MSG], out_string[MAX_MSG];
pthread_t slave_server;
int threadID, ret;
struct timeval tv;

	servAddr.sin_family = AF_INET;
	servAddr.sin_addr.s_addr = htonl(INADDR_ANY);
	servAddr.sin_port = htons(SERVER_PORT);

	/* create socket */
	sd = socket(AF_INET, SOCK_STREAM, 0);
  

   	if(sd<0)
		{
    		perror("socket ");
    		}

	/* allow multiple sockets to use the same PORT number */ 
	if (setsockopt(sd,SOL_SOCKET,SO_REUSEADDR,&yes,sizeof(yes)) < 0)
		{ 
		perror("Reusing ADDR failed");
		exit(1); 
		} 


	/* bind server port */
	if(bind(sd, (struct sockaddr *) &servAddr, sizeof(servAddr))<0)
		{
		perror("bind");
		}

	if (listen(sd,5) == -1)
		{
		perror("listen");
		}

	//listen for, and accept requests, forever
	while(1)
		{

		//printf("waiting for data on port TCP %u\n",SERVER_PORT);

		cliLen = sizeof(cliAddr);
		newSd = accept(sd, (struct sockaddr *) &cliAddr, &cliLen);

    		if(newSd<0) 
			{
      			perror("accept");
      			}

		ret = packet_handler(newSd);
		if (ret=NOK)
			{
			printf("Message Not OK\n");
			}	            

		close(newSd);
			
		} // while(1)

}//server_entry
		

/**********************************************
*Title: packet_handler
*Author: Chris Johnson
*date 7/14/05
*Description: this is where the server delegates most
* of the work of sending, and receiving data.
*
*Args: newSd = socket descriptor	
*
*Returns:
*			0(OK) = SUCCESS
*			1(NOK) = ERROR
**********************************************/
int packet_handler(int newSd)
{
pthread_t temp;
char in_string[MAX_MSG], out_string[MAX_MSG];
char buffer[5];
int rstat;
	
	memset(in_string,0,MAX_MSG);  // clear the input string
	rstat=recv(newSd,&in_string,sizeof(in_string),0);

	if (rstat == -1) 
		{
		perror("recv");
		return NOK;
		}

	if (rstat == 0) 
		{
		printf("Socket disconnected.\n");
		return NOK;
		}

	if (msg_handler(in_string, out_string) == NOK)
		{
		printf("error\n");
		return NOK;
		}
	else
		{
		int tommy;
		tommy = send(newSd, out_string, strlen(out_string), 0);
		if (tommy < 0)
			{
			printf("cannot send data ");
			return NOK;
			}
	  	}

return OK;
		
		
} //end handler()


/**********************************************
*Title: parse_line
*Author: Chris Johnson
*date 6/22/05
*Description: this parses the message string into
*  a structure for later use.
*
*Args: new_message = message struct to populate
*	line = pointer to string containing 
*		received message	
*
*Returns:
*			0(OK) = SUCCESS
*			1(NOK) = ERROR
**********************************************/
int parse_line(TCS_MESSAGE *new_message, char *line) 
{
char input[MAX_MSG], reply[MAX_MSG];
unsigned short int PIDint;
int numArgs;
char TelID[9], SysID[6], PackID[3], command[7], subCmd[10];
char *nextTok;
char stoken[4];

	stoken[0] = ' ';
	stoken[1] = '\t';
	stoken[2] = '\n';
	stoken[3] = '\0';
	 	
 	strncat(reply, line, strlen(line));
		
 	//peel off the header first
 	nextTok = (char *)strtok(line, stoken);
 	if (nextTok == NULL){return NOK;}
 	sprintf(new_message->telID, "%s", nextTok);//telescope ID
 	
 	nextTok = (char *)strtok(NULL, stoken);
 	if (nextTok == NULL){return NOK;}
 	sprintf(new_message->sysID, "%s", nextTok);//system ID
 
  	nextTok = (char *)strtok(NULL, stoken);
  	if (nextTok == NULL){return NOK;}
 	new_message->pID = atoi(nextTok);//packet ID
 	
 	nextTok = (char *)strtok(NULL, stoken);
 	if (nextTok == NULL){return NOK;}
 	sprintf(new_message->cmd, "%s", nextTok);//command
 	
	//now parse the command arguments
	numArgs = 0;
	nextTok = (char *)strtok(NULL, stoken);
	while(nextTok != NULL)
		{
 		sprintf(new_message->cmdArgs[numArgs], "%s", nextTok);//argument
 		numArgs++;
 		nextTok = (char *)strtok(NULL, stoken);
		}

	//null out the rest so we don't get bad data
	while(numArgs < MAX_ARGS)
		{
		new_message->cmdArgs[numArgs][0] = '\0';
		numArgs++;
		}
	return OK;
			
}//end parse_line()
	
/**********************************************
*Title: msg_handler
*Author: Chris Johnson
*date 6/22/05
*Description: this is the general message handling
*	routine...  makes calls to parse and check
* validity...  switches between
* strings and structures... then makles requests
* to the astro engine and builds out_string
* for the reply
*
*Args: in_string = received string
*	out_string = string to be sent 
*
*Returns:
*			0(OK) = SUCCESS
*			1(NOK) = ERROR
*
**********************************************/
int msg_handler(char *in_string, char *out_string) 
	{
	TCS_MESSAGE new_message, ret_message;
	int err;

	//init all data to bogus values
	init_data(&ret_message);
  	sprintf(out_string, "%c", '\0');

	//parse incomming message into a structure for later processing
	if(parse_line(&new_message, in_string) != OK)
		{
    		sprintf(ret_message.cmd, CMD_ERR);
		gen_string(&ret_message, out_string);
	  	return NOK;
		}
	//is this for this telescope?
	
	if(strcmp( new_message.telID, TELESCOPE_ID ) != 0)
		{
		sprintf(ret_message.cmd, TELID_ERR);
		gen_string(&ret_message, out_string);
		printf("BAD TEL ID %s is not %s\n", new_message.telID, TELESCOPE_ID);
	        return NOK;
		}
	if(strcmp(new_message.sysID, SYSTEM_ID) != 0)
		{
		printf("BAD SYS ID\n");
		sprintf(ret_message.cmd, SYSID_ERR);
		gen_string(&ret_message, out_string);
	  	return NOK;
		}

//pthread_mutex_lock(&weather_mutex);//////////////////////////LOCK/////////////////////
  	//is it a request?
	if(strcmp(new_message.cmd, CMD_REQ) == 0)
		{
		
		cpy_header(&new_message, &ret_message);
        	//its a REQUEST... call daves hook
		if((err=call_daves_request_hook(new_message.cmdArgs, ret_message.cmdArgs)) == OK)
			{
			sprintf(ret_message.cmd," ");
			}
      		else
			{
			sprintf(ret_message.cmd, "%s %d", CMD_REQ_ERR,err);
			}
		gen_string(&ret_message, out_string);
	  }
	  
	//is it a command?
	else if(strcmp(new_message.cmd, CMD_CMD) == 0)
		{
		cpy_header(&new_message, &ret_message);
    		//its a COMMAND... call daves hook
		if((err=call_daves_command_hook(new_message.cmdArgs)) == OK)
			{
			sprintf(ret_message.cmd, CMD_SUCCESS);
			}
    		else
			{
			sprintf(ret_message.cmd, "%s %d",CMD_CMD_ERR,err);
			}
		gen_string(&ret_message, out_string);
		}
  	//if its not a known command return unknown
	else
		{
		cpy_header(&new_message, &ret_message);
		sprintf(ret_message.cmd, CMD_UNKNOWN);
		gen_string(&ret_message, out_string);
		}
//pthread_mutex_unlock(&weather_mutex);//////////////////////////UNLOCK/////////////////////		
	return OK;
	}
	

/**********************************************
*Title: gen_string
*Author: Chris Johnson
*date 6/22/05
*Description: converts the structure back to a string
*
**Args: ret_message = message struct to cpnvert
*	out_string = string to be sent	
*
*Returns:
*
**********************************************/
void gen_string(TCS_MESSAGE *ret_message, char *out_string)
{
int x;
char buffer[4];
char delim = ' ';
char *f;

	sprintf(buffer, "%i", ret_message->pID);

	//first work out the header
	strncat(out_string, ret_message->telID, strlen(ret_message->telID));
	strncat(out_string, &delim, 1);
	strncat(out_string, ret_message->sysID, strlen(ret_message->sysID));
	strncat(out_string, &delim, 1);
  	strcat(out_string, buffer);
  	strncat(out_string, &delim, 1);
	strncat(out_string, ret_message->cmd, strlen(ret_message->cmd));
	strncat(out_string, &delim, 1);

	//parse through the arguments
	for(x=0;x<MAX_ARGS;x++)
		{
		if(strcmp(ret_message->cmdArgs[x], NODATA) != 0)
			{
			strncat(out_string, ret_message->cmdArgs[x], strlen(ret_message->cmdArgs[x]));
			strncat(out_string, &delim, 1);
			}
		}

}


/**********************************************
*Title: init_data
*Author: Chris Johnson
*date 6/22/05
*Description: inits a structure with bogus data
*
*Args: ret_message = struct to cleared
*
*Returns:
*
**********************************************/
void init_data(TCS_MESSAGE *ret_message)
{
int x;

	sprintf(ret_message->telID, NODATA);
	sprintf(ret_message->sysID, NODATA);
	ret_message->pID =  0;
	sprintf(ret_message->cmd, NODATA);

	for(x=0;x<MAX_ARGS;x++)
		{
		sprintf(ret_message->cmdArgs[x], NODATA);
		}
	
}

/**********************************************
*Title: cpy_header
*Author: Chris Johnson
*date 6/22/05
*Description: copy's the incoming header to the
* outgoing structure
*
*Args: new_message = message struct to populate
*	ret_message = message to copy from	
*
*Returns:
*
**********************************************/
void cpy_header(TCS_MESSAGE *new_message, TCS_MESSAGE *ret_message)
{
  	sprintf(ret_message->telID, "%s", new_message->telID);
	sprintf(ret_message->sysID, "%s", new_message->sysID);
	ret_message->pID = new_message->pID;
	sprintf(ret_message->cmd, "%s", new_message->cmd);
}

/**********************************************
*Title: call_daves_request_hook
*Author: Chris Johnson
*date 6/22/05
*Description: just a place to put a call to
*  harveys astro engine commands...
*
*Args: inArgs = arguments passed in from client
*	outArgs = data passed back to client
*
*Returns:
*			0(OK) = SUCCESS
*			1(NOK) = ERROR
*
**********************************************/
int call_daves_request_hook(ARG_ARRAY inArgs, ARG_ARRAY outArgs)
{
    
if (strcmp(inArgs[0], "WHO") == 0)  // 
	{
	sprintf(outArgs[0],"WHO_FARTED");
	return OK;
	}
/*
if (strcmp(inArgs[0], "TEMP_IN") == 0)  // 
	{
	sprintf(outArgs[0],"%f", weather_wx.temp_in);
	return OK;
	}

if (strcmp(inArgs[0], "TEMP_OUT") == 0)  // 
	{
	sprintf(outArgs[0],"%f", weather_wx.temp_out);
	return OK;
	}

if (strcmp(inArgs[0], "HUM_IN") == 0)  // 
	{
	sprintf(outArgs[0],"%f", weather_wx.hum_in);
	return OK;
	}

if (strcmp(inArgs[0], "HUM_OUT") == 0)  // 
	{
	sprintf(outArgs[0],"%f", weather_wx.hum_out);
	return OK;
	}

if (strcmp(inArgs[0], "WIND_SP") == 0)  // 
	{
	sprintf(outArgs[0],"%f", weather_wx.wind_sp);
	return OK;
	}

if (strcmp(inArgs[0], "WIND_DIR") == 0)  // 
	{
	sprintf(outArgs[0],"%f", weather_wx.wind_dir);
	return OK;
	}

if (strcmp(inArgs[0], "PRESSURE") == 0)  // 
	{
	sprintf(outArgs[0],"%f", weather_wx.pressure);
	return OK;
	}

if (strcmp(inArgs[0], "PRECIP") == 0)  // 
	{
	sprintf(outArgs[0],"%f", weather_wx.precip);
	return OK;
	}

if (strcmp(inArgs[0], "ALL") == 0)  // 
	{
	sprintf(outArgs[0],"%s %f %f %f %f %f %f %f %f ", 
		weather_wx.who, weather_wx.temp_in, weather_wx.temp_out, weather_wx.hum_in, 
		weather_wx.hum_out, weather_wx.wind_sp, weather_wx.wind_dir, weather_wx.pressure,  			weather_wx.precip);
	return OK;
	}*/

return NOK;
}

/**********************************************
*Title: call_daves_command_hook
*Author: Chris Johnson
*date 6/22/05
*Description: just a place to put a call to
*  harveys astro engine commands...
*
*Args: inArgs = arguments passed in from client
*	outArgs = data passed back to client
*
*returns:
*			1 = error
*			0 = success
**********************************************/
int call_daves_command_hook(ARG_ARRAY inArgs)
{

	if (strcmp(inArgs[0], "FART") == 0)
		{
		return OK;
		}
return NOK;
}



/**********************************************
*Title: printmessage
*Author: Chris Johnson
*date 6/22/05
*Description: for test only...  prints the
* entire TCS_MESSAGE structure
*
*Args: inArgs = arguments passed in from client
*	outArgs = data passed back to client
*
*Returns:
*
**********************************************/

void printmessage(TCS_MESSAGE *new_message)
{
int x;

	printf("printmessage -- dump of contents\n");
	printf("%s\n", new_message->telID);
	printf("%s\n", new_message->sysID);
	printf("%i\n", new_message->pID);
	printf("%s\n", new_message->cmd);

	for(x=0;x<MAX_ARGS;x++)
		{
		if(new_message->cmdArgs[x][0] != '\0')
			{
				printf("arg%i: %s\n", x, new_message->cmdArgs[x]);
			}
		}

}
