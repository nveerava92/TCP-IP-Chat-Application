#ifndef GLOBAL_H_
#define GLOBAL_H_

#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <netdb.h>
#include <sys/select.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <errno.h>

#include "circular_buffer.h"
#include "packet.h"

#define TRUE 1
#define FALSE 0
#define HOSTNAME_LEN 128
#define PATH_LEN 256
#define MAX_USERS 100
#define MAX_LIVE_USERS 5

#define USER_INPUT_SIZE 1024
#define NI_MAXHOST 1025

#define BACKLOG_BUFFER_SIZE 100

typedef enum {
	OFFLINE,
	ONLINE,
} login_status;

typedef struct backlog_msg
{
	char sender_ip[INET_ADDRSTRLEN];
	char message[257];
	int is_broadcast;
} backlog_msg;

typedef struct user {
	int fd;
	CircularBuffer *cb;
	char ip[INET_ADDRSTRLEN];
	unsigned short port;
	login_status status;
	char hostname[NI_MAXHOST];
	unsigned int msg_sent;
	unsigned int msg_recv;
	char *blocked_list[MAX_LIVE_USERS];
	backlog_msg *backlog_list[BACKLOG_BUFFER_SIZE];
	size_t backlog_count;
} user;

void print_author(void);
void print_ip(void);
void get_self_ip(char *ip);
void print_port(int fd);
int compare_users(const void *a, const void *b);

ssize_t generate_users_string(char **msg, int count, struct user *users[]);
ssize_t parse_users_string(const char *msg, size_t msg_len, user *users[]);
ssize_t generate_backlog_string(char **msg, struct user *client);
ssize_t parse_backlog_string(char *data_string);
ssize_t generate_msg_string(char **msg, char *send_ip, char *message);
ssize_t parse_msg_string(char *msg, char **dest_ip, char **message, int send);

int old_user(struct user **users, const char *client_ip, unsigned short client_port, const char *hostname);
void list_users(struct user **users, size_t user_count);
int clear_blist(struct user *client);
int add_msg_blist(char *send_ip, char *message, struct user *receiver, int is_broadcast);
int block (const char *ip_address, struct user* client);
int unblock (const char *ip_address, struct user* client);

#endif
