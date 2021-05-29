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
typedef struct
{
    char host[32];
    int roomid;
} room_t;

typedef struct
{
    struct sockaddr_in address;
    room_t room;
    int sockfd;
    int uid;
    char name[32];
} client_t;

int room_exist(int roomid);
void newuser_setting();
void check_room();
void print_client_addr(struct sockaddr_in addr);
void error(const char *msg);
void server_config();
void send_message(char *s, int uid, int roomid);
void listen_to_connection();
void set_listenfd();
void bind_sockfd();
void *handle_client(void *arg);
void add_client(client_t *cl);
void remove_client(int uid);
void send_message_same_room(char *s, int uid, int roomid);
void send_message_current_client(char *s, int uid);
void client_name_handler(void *arg, int *leave_flag);
void client_room_handler(void *arg, int *leave_flag);
void room_list_append(int roomid, char *name);
void room_list_init();
void send_file(int uid);
