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

void catch_ctrl_c_and_exit(int sig)
{
	flag = 1;
}

int username_handler()
{
	printf("Please enter your username: ");
	fgets(name, 32, stdin);
	str_trim_lf(name, strlen(name));
	if (strlen(name) > 32 || strlen(name) < 2)
	{
		printf("Name must be less than 30 and more than 2 characters.\n");
		bzero(name, sizeof(name));
		close(sockfd);
		return EXIT_FAILURE;
	}
	// Send name
	send(sockfd, name, 32, 0);
	return EXIT_SUCCESS;
}

int option_handler()
{
	fgets(option, 100, stdin);
	str_trim_lf(option, strlen(option));
	int opt = atoi(option);
	if (opt != 1 && opt != 2)
	{
		printf("We have only two options ( 1 or 2), please enter again \n > ");
		close(sockfd);
		bzero(name, 32);
		bzero(option, 100);
		bzero(roomid, 100);
		return EXIT_FAILURE;
	}
	else
	{
		send(sockfd, option, 100, 0);
	}
	
	char message[LENGTH] = {};
	if (recv(sockfd, message, LENGTH, 0))
	{
		printf("%s\n", message);
	}
	else
	{
		perror("Error on receiving");
	}
	//clear message
	memset(message, 0, sizeof(message));
	// bzero(message, sizeof(message));
	bzero(option, 100);
	return EXIT_SUCCESS;
}
int roomid_handler()
{
	fgets(roomid, 100, stdin);
	str_trim_lf(roomid, strlen(roomid));

	send(sockfd, roomid, 100, 0);

	char message[LENGTH] = {};
	while (1)
	{
		if (recv(sockfd, message, LENGTH, 0))
		{
			printf("%s\n", message);
			// str_overwrite_stdout();
			break;
		}
		else if (recv(sockfd, message, LENGTH, 0) == 0)
		{
			printf("Nothing avaiable to receive \n");
			break;
		}
		else
		{
			// -1
			perror("Error on receiving abc \n");
		}
		//clear message
		memset(message, 0, sizeof(message));
		// bzero(message, sizeof(message));
		bzero(option, 100);
	}
	return EXIT_SUCCESS;
}
void send_msg_handler()
{
	char message[LENGTH] = {};
	while (1)
	{
		str_overwrite_stdout();
		fgets(message, LENGTH, stdin);
		str_trim_lf(message, LENGTH);

		if (strcmp(message, "exit") == 0)
		{
			break;
		}
		else
		{
			sprintf(buffer, "%s: %s \n", name, message);
			send(sockfd, buffer, strlen(buffer), 0);
		}

		memset(message, 0, sizeof(message));
		bzero(buffer, LENGTH + 32 + 3);
	}
	catch_ctrl_c_and_exit(2);
}

void recv_msg_handler()
{
	char message[LENGTH] = {};
	while (1)
	{
		int receive = recv(sockfd, message, LENGTH, 0);
		if (receive > 0)
		{
			printf("%s", message);
			str_overwrite_stdout();
		}
		else if (receive == 0)
		{
			break;
		}
		else
		{
			// -1
			perror("Error on receiving");
		}
		//clear message
		memset(message, 0, sizeof(message));
		// bzero(message, sizeof(message));
	}
}

int main(int argc, char **argv)
{
	if (argc != 2)
	{
		printf("Usage: %s <port>\n", argv[0]);
		return EXIT_FAILURE;
	}

	char *ip = "127.0.0.1";
	int port = atoi(argv[1]);
	struct sockaddr_in server_addr;

	/* Socket settings */
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = inet_addr(ip);
	server_addr.sin_port = htons(port);

	// Connect to Server
	int err = connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr));
	if (err < 0)
	{
		error("ERROR: connect\n");
		return EXIT_FAILURE;
	}
	//this use for catch ctrl_c then exit
	signal(SIGINT, catch_ctrl_c_and_exit);

	if (username_handler())
		return EXIT_FAILURE;

	printf("Please choose your option (1 or 2): \n");
	printf("1. Create a new room by entering roomid \n");
	printf("2. Join an existing room by entering roomid \n");
	str_overwrite_stdout();

	if (option_handler())
	{
		close(sockfd);
		bzero(name, 32);
		bzero(option, 100);
		bzero(roomid, 100);
		return EXIT_FAILURE;
	}

	printf("Enter room id: \n");
	str_overwrite_stdout();

	if (roomid_handler())
	{
		close(sockfd);
		bzero(name, 32);
		bzero(option, 100);
		bzero(roomid, 100);
		return EXIT_FAILURE;
	}

	printf("=== WELCOME TO THE CHATROOM === \n");

	pthread_t send_msg_thread;
	if (pthread_create(&send_msg_thread, NULL, (void *)send_msg_handler, NULL) != 0)
	{
		printf("ERROR: pthread\n");
		return EXIT_FAILURE;
	}

	pthread_t recv_msg_thread;
	if (pthread_create(&recv_msg_thread, NULL, (void *)recv_msg_handler, NULL) != 0)
	{
		printf("ERROR: pthread\n");
		return EXIT_FAILURE;
	}

	while (1)
	{
		if (flag)
		{
			printf("\nBye\n");
			break;
		}
	}

	close(sockfd);
	bzero(name, sizeof(name));
	return EXIT_SUCCESS;
}
