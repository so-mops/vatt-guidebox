#include <netdb.h> 
#include <stdio.h> 
#include <stdlib.h> 
#include <string.h> 
#include <sys/socket.h> 
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

void lantronix_reset()
{
int sockfd, connfd, x;
char serialfix=128; 
char buff[500];
struct sockaddr_in servaddr, cli; 
  
	// socket create and varification 
	sockfd = socket(AF_INET, SOCK_STREAM, 0); 
	if (sockfd == -1) 
		{ 
		printf("socket creation failed...\n"); 
		exit(0); 
		} 
	else
	
	printf("Socket successfully created..\n"); 
	bzero(&servaddr, sizeof(servaddr)); 
  
	// assign IP, PORT 
	servaddr.sin_family = AF_INET; 
	servaddr.sin_addr.s_addr = inet_addr("10.130.133.24"); 
	servaddr.sin_port = htons(9999); 
  
	// connect the client socket to server socket 
	if (connect(sockfd, (struct sockaddr*)&servaddr, sizeof(servaddr)) != 0) 
		{ 
		printf("connection with the server failed...\n"); 
		exit(0); 
		} 
	else
		printf("connected to the server..\n"); 

	

	//first wait for the connection message
	sleep(2);
	memset(buff, 0, sizeof(buff));
	x=recv(sockfd, buff, 500, MSG_DONTWAIT);
	//printf("x=%i chars\n", x);
	//printf("buff=%s\n", buff);

	//send a carriage return to enter the menu
	//then wait for the menu items
	write(sockfd, "\r", 1);
	sleep(2);
	memset(buff, 0, sizeof(buff));
	x=recv(sockfd, buff, sizeof(buff), MSG_DONTWAIT);
	//printf("x=%i chars\n", x);
	//printf("buff=%s\n", buff);

	//select option 9 "save and exit", and 
	//send another carriage return
	write(sockfd, "9\r", 2);
	close(sockfd);

	sleep(5);


}

void main()
{
lantronix_reset();
}


