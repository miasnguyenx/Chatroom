#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <pthread.h>
#include <sys/types.h>
#include <signal.h>

void str_overwrite_stdout() {
    printf("\r%s", "> ");
    fflush(stdout);
}

void print_client_addr(struct sockaddr_in addr)
{
	printf("%d.%d.%d.%d",
		   addr.sin_addr.s_addr & 0xff,
		   (addr.sin_addr.s_addr & 0xff00) >> 8,
		   (addr.sin_addr.s_addr & 0xff0000) >> 16,
		   (addr.sin_addr.s_addr & 0xff000000) >> 24);
}
void str_trim_lf(char *arr, int length)
{
	int i;
	for (i = 0; i < length; i++)
	{ // trim \n
		if (arr[i] == '\n')
		{
			arr[i] = '\0';
			break;
		}
	}
}
// void client_name_handler(void *arg, int *leave_flag)
// {
//     char name[32];
//     char buff_out[BUFFER_SZ];
//     client_t *cli = (client_t *)arg;
//     // Get the name from message
//     if (recv(cli->sockfd, name, 32, 0) <= 0 || strlen(name) < 2 || strlen(name) >= 32 - 1)
//     {
//         printf("Didn't enter the name or enter an unappropriate name.\n");
//         *leave_flag = 1;
//     }
//     else
//     {
//         strcpy(cli->name, name);
//         sprintf(buff_out, "%s has joined\n", cli->name);
//         printf("%s", buff_out);
//         //send_message
//         send_message_same_room(buff_out, cli->uid, cli->roomid);
//     }

//     bzero(buff_out, BUFFER_SZ);
// }

// void client_room_handler(void *arg, int *leave_flag)
// {
//     char roomid[OPTION_SZ];
//     char option[OPTION_SZ];
//     char buff_out[BUFFER_SZ];
//     int opt;
//     int rid;
//     int n;
//     client_t *cli = (client_t *)arg;

//     n = recv(cli->sockfd, option, sizeof(option), 0);
//     if (n <= 0)
//     {
//         perror("ERROR on receiving");
//     }
//     opt = atoi(option);

//     n = recv(cli->sockfd, roomid, sizeof(roomid), 0);
//     if (n <= 0)
//     {
//         perror("ERROR on receiving");
//     }
//     rid = atoi(roomid);

//     if (opt == 1)
//     {

//         if (rid <= 0 || rid >= 1000)
//         {
//             printf("Didn't enter the roomid or enter an unappropriate roomid.\n");
//             *leave_flag = 1;
//         }
//         if (room_exist(rid))
//         {
//             printf("Your roomid already exists.\n");
//             *leave_flag = 1;
//         }
//         else
//         {
//             cli->roomid = rid;
//             sprintf(buff_out, "you has joined room %d\n", cli->roomid);
//             printf("%s", buff_out);
//             //send_message
//             send_message_same_room(buff_out, cli->uid, cli->roomid);
//         }
//         bzero(buff_out, BUFFER_SZ);
//     }
//     else if (opt == 2)
//     {
//     }
// }


