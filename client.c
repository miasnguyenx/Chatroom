#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <netdb.h>
#include "server.h"
#include "config.c"
#include "utility.c"

// Global variables
volatile sig_atomic_t flag = 0;
int sockfd = 0;
char name[32] = {};
char option[100] = {};
char roomid[100] = {};
char buffer[LENGTH + 32 + 3] = {};
int n;

char roomListMessage[LENGTH] = {};
char recvMessage[LENGTH] = {};

char* recv_room_list()
{
	sleep(1);
	int receive = recv(sockfd, roomListMessage, LENGTH, 0);
	char *return_message = malloc ( sizeof(char) * strlen(roomListMessage) );
	if (receive > 0)
	{
		strcpy(return_message, roomListMessage);

	}
	memset(roomListMessage, 0, sizeof(roomListMessage));
	return return_message;
}

void connectServer(char *ipAddress, int port)
{
	struct hostent *server;
	struct sockaddr_in server_addr;
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	server = gethostbyname(ipAddress);
	
	/* Socket settings */
	bzero((char *)&server_addr, sizeof(server_addr));

	server_addr.sin_family = AF_INET;

	bcopy((char *)server->h_addr,
		  (char *)&server_addr.sin_addr.s_addr,
		  server->h_length);

	server_addr.sin_port = htons(port);

	// Connect to Server
	int err = connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr));

	recv_room_list();
	
	while (1) {
		if (flag) {
			close(sockfd);
			break;
		}
	}
}

void catch_ctrl_c_and_exit(int sig)
{
	char send_message[LENGTH] = {};
	strcpy(send_message, "exit");
	str_trim_lf(send_message, LENGTH);
	sprintf(buffer, "%s",send_message);
	send(sockfd, buffer, strlen(buffer), 0);
	flag = 1;
}

void username_handler(char input[]) {
	strcpy(name, input);
	str_trim_lf(name, strlen(name));
	send(sockfd, name, 32, 0);
}

void option_handler(char input[]) {
	strcpy(option, input);
	str_trim_lf(option, strlen(option));
	send(sockfd, option, 100, 0);
}

void room_handler(char input[]) {
	strcpy(roomid, input);
	str_trim_lf(roomid, strlen(roomid));
	send(sockfd, roomid, 100, 0);
}

void send_msg(char input[])
{
	char send_message[LENGTH] = {};
	strcpy(send_message, input);
	str_trim_lf(send_message, LENGTH);
	sprintf(buffer, "%s: %s \n", name, send_message);
	send(sockfd, buffer, strlen(buffer), 0);
	bzero(buffer, LENGTH + 32 + 3);
}

char* recv_msg() {
	int receive = recv(sockfd, recvMessage, LENGTH, 0);
	char *return_message = malloc ( sizeof(char) * strlen(recvMessage) );
	if (receive > 0) {
		strcpy(return_message, recvMessage);
	}
	bzero(recvMessage, sizeof(recvMessage));
	return return_message;
}
