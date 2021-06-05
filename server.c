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
#include "server.h"
#include "config.c"
#include "utility.c"

client_t *clients[MAX_CLIENTS];
pthread_mutex_t clients_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t rooms_mutex = PTHREAD_MUTEX_INITIALIZER;
// struct sockaddr_in serv_addr;
// struct sockaddr_in cli_addr;
// int option = 1;
// int connfd = 0;
// int listenfd = 0;
static int uid = 10;
int cli_count;
char room_list[LENGTH];

void error(const char *msg)
{
    perror(msg);
    exit(1);
}

/* ---------- clean ----------- */
void send_message_same_room(char *s, int uid, int roomid)
{
    //lock anything in this group of code
    pthread_mutex_lock(&clients_mutex);

    for (int i = 0; i < MAX_CLIENTS; ++i)
    {
        if (clients[i])
        {
            // client has roomid, which indicated that client has enter the roomid
            // roomid must different from zero, otherwise clients which haven't completed step one will catch an error
            if (clients[i]->uid != uid && clients[i]->room.roomid == roomid && roomid != 0)
            {
                if (write(clients[i]->sockfd, s, strlen(s)) < 0)
                {
                    perror("ERROR: write to descriptor failed");
                    break;
                }
            }
        }
    }

    pthread_mutex_unlock(&clients_mutex);
}
void send_message_current_client(char *s, int uid)
{
    //lock anything in this group of code
    pthread_mutex_lock(&clients_mutex);
    for (int i = 0; i < MAX_CLIENTS; ++i)
    {
        if (clients[i])
        {
            if (clients[i]->uid == uid)
            {
                if (write(clients[i]->sockfd, s, LENGTH) < 0)
                {
                    perror("ERROR: write to descriptor failed");
                    break;
                }
            }
        }
    }
    pthread_mutex_unlock(&clients_mutex);
}
void send_file(int uid)
{
    char buffer[LENGTH];
    FILE *fp = fopen("roomlist.txt", "r");
    char data[LENGTH];
    while (fgets(data, LENGTH, fp) != NULL)
    {
        strcat(buffer,data);
    }
    send_message_current_client(buffer, uid);
    bzero(data, LENGTH);
    fclose(fp);
}

void room_list_append(int roomid, char *name)
{
    FILE *fp;

    /* open the file for writing*/
    fp = fopen("roomlist.txt", "a");
    fprintf(fp, "Room ID: %d hosted by %s\n", roomid, name);
    fclose(fp);
}

void *handle_client(void *arg)
{
    char buff_out[BUFFER_SZ];
    int leave_flag = 0;
    client_t *cli = (client_t *)arg;

    cli_count++;
    char name[32];
    
    send_file(cli->uid);

    if (recv(cli->sockfd, name, 32, 0) <= 0 || strlen(name) < 2 || strlen(name) >= 32 - 1)
    {
        printf("Didn't enter the name or enter an unappropriate name.\n");
        leave_flag = 1;
    }
    else
    {
        strcpy(cli->name, name);
        sprintf(buff_out, "%s has joined \n", cli->name);
        printf("%s", buff_out);

        bzero(buff_out, BUFFER_SZ);
    }

    char opt[OPTION_SZ];
    int opt_int;

    int receive = recv(cli->sockfd, opt, 100, 0);
    if (receive <= 0 || atoi(opt) > 2 || atoi(opt) < 1)
    {
        printf("Entered the wrong option or didn't enter \n");
        bzero(opt, 100);
        leave_flag = 1;
    }
    else if (receive > 0)
    {
        str_trim_lf(opt, strlen(opt));
        opt_int = atoi(opt);

        sprintf(buff_out, "You has selected option %d <-- from server", atoi(opt));
        if (opt_int == 1)
            printf("Username: '%s' want to create a room \n", name);
        else
            printf("Username: '%s' want to join a room \n", name);

        send_message_current_client(buff_out, cli->uid);
        bzero(buff_out, BUFFER_SZ);
        bzero(opt, 100);
    }

    char roomid[OPTION_SZ];
    int rid_int;
    receive = recv(cli->sockfd, roomid, 100, 0);

    if (receive > 0)
    {
        str_trim_lf(roomid, strlen(roomid));
        rid_int = atoi(roomid);

        if (opt_int == 1)
        {
            if (rid_int <= 0 || rid_int >= 1000)
            {
                printf("Didn't enter the roomid or enter an unappropriate roomid.\n");
                fflush(stdout);
                leave_flag = 1;
            }
            else if (room_exist(rid_int))
            {
                printf("Client's roomid already exists.\n");
                fflush(stdout);
                send_message_current_client("Your entered roomid already exists <-- from server", cli->uid);
                leave_flag = 1;
            }
            else
            {
                cli->room.roomid = rid_int;
                room_list_append(rid_int, name);
                printf("Created a room number %d for client \n", rid_int);
                // fflush(stdout);
                sprintf(buff_out, "You has created a room id %d <-- from server \n", rid_int);
                send_message_current_client("You has created a room <-- from server \n", cli->uid);
            }
        }
        else if (opt_int == 2)
        {
            if (!room_exist(rid_int))
            {
                printf("Room entered: %d doesn't exist \n", rid_int);
                fflush(stdout);
                send_message_current_client("your entered roomid doesn't exist \n", cli->uid);
            }
            else
            {
                cli->room.roomid = rid_int;
                printf("Joined username '%s' to room %d \n", name, rid_int);
                sprintf(buff_out, "You has joined room %d \n", rid_int);
                send_message_current_client(buff_out, cli->uid);
            }
        }
    }
    bzero(roomid, 100);
    bzero(buff_out, BUFFER_SZ);

    //each thread will serve the connection from one client only
    //get message from each of clients and send to all of them
    while (1)
    {
        if (leave_flag)
        {
            break;
        }
        //receive and send
        int receive = recv(cli->sockfd, buff_out, BUFFER_SZ, 0);
        //If client enter exit we need to terminate the connection from client side
        if (receive > 0 && strcmp(buff_out, "exit") != 0)
        {
            if (strlen(buff_out) > 0)
            {
                send_message_same_room(buff_out, cli->uid, cli->room.roomid);

                str_trim_lf(buff_out, strlen(buff_out));
                printf("%s <-- %s\n", buff_out, cli->name);
            }
        }
        else if (receive == 0 || strcmp(buff_out, "exit") == 0)
        {
            sprintf(buff_out, "%s has left\n", cli->name);
            printf("%s", buff_out);
            send_message_same_room(buff_out, cli->uid, cli->room.roomid);
            leave_flag = 1;
        }
        else
        {
            printf("ERROR: -1\n");
            leave_flag = 1;
        }

        bzero(buff_out, BUFFER_SZ);
    }

    /* Delete client from queue and yield thread */
    close(cli->sockfd);
    remove_client(cli->uid);
    free(cli);
    cli_count--;
    pthread_detach(pthread_self());

    return NULL;
}

void add_client(client_t *cl)
{
    pthread_mutex_lock(&clients_mutex);

    for (int i = 0; i < MAX_CLIENTS; ++i)
    {
        if (!clients[i])
        {
            clients[i] = cl;
            break;
        }
    }

    pthread_mutex_unlock(&clients_mutex);
}
void remove_client(int uid)
{
    pthread_mutex_lock(&clients_mutex);

    for (int i = 0; i < MAX_CLIENTS; ++i)
    {
        if (clients[i])
        {
            if (clients[i]->uid == uid)
            {
                clients[i] = NULL;
                break;
            }
        }
    }

    pthread_mutex_unlock(&clients_mutex);
}

int room_exist(int roomid)
{
    pthread_mutex_lock(&clients_mutex);

    for (int i = 0; i < MAX_CLIENTS; ++i)
    {
        if (clients[i])
        {
            if (clients[i]->room.roomid == roomid && roomid != 0)
            {
                pthread_mutex_unlock(&clients_mutex);
                return 1;
            }
        }
    }
    pthread_mutex_unlock(&clients_mutex);
    return 0;
}

int main(int argc, char **argv)
{
    pthread_mutex_init(&clients_mutex, NULL);
    pthread_mutex_init(&rooms_mutex, NULL);
    FILE *fp = fopen("roomlist.txt","w");
    fprintf(fp, "Current existing rooms: \n");
    fclose(fp);
    char *ip = "127.0.0.1";
    int port = atoi(argv[1]);
    int option = 1;
    int listenfd = 0, connfd = 0;
    struct sockaddr_in serv_addr;
    struct sockaddr_in cli_addr;
    pthread_t tid;

    /* Socket settings */
    listenfd = socket(AF_INET, SOCK_STREAM, 0);
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = inet_addr(ip);
    serv_addr.sin_port = htons(port);

    /* Ignore pipe signals */
    signal(SIGPIPE, SIG_IGN);

    if (setsockopt(listenfd, SOL_SOCKET, (SO_REUSEPORT | SO_REUSEADDR), (char *)&option, sizeof(option)) < 0)
    {
        perror("ERROR: setsockopt failed");
        return EXIT_FAILURE;
    }

    /* Bind */
    if (bind(listenfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    {
        perror("ERROR: Socket binding failed");
        return EXIT_FAILURE;
    }

    /* Listen */
    if (listen(listenfd, 10) < 0)
    {
        perror("ERROR: Socket listening failed");
        return EXIT_FAILURE;
    }

    printf("=== WELCOME TO THE CHATROOM ===\n");

    while (1)
    {
        //The listenToConnection will block the program until received a signal of connection

        socklen_t clilen = sizeof(cli_addr);
        connfd = accept(listenfd, (struct sockaddr *)&cli_addr, &clilen);

        /* Check if max clients is reached */
        if ((cli_count + 1) == MAX_CLIENTS)
        {
            printf("Max clients reached. Rejected: ");
            print_client_addr(cli_addr);
            printf(":%d\n", cli_addr.sin_port);
            close(connfd);
            continue;
        }

        /* Client settings */
        client_t *cli = (client_t *)malloc(sizeof(client_t));
        cli->address = cli_addr;
        cli->sockfd = connfd;
        cli->uid = uid++;

        /* Add client to the queue and fork thread */
        add_client(cli);
        pthread_create(&tid, NULL, &handle_client, (void *)cli);

        /* Reduce CPU usage */
        sleep(1);
    }
    close(connfd);
    close(listenfd);

    return EXIT_SUCCESS;
}
