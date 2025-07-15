#include "global.h"
#include "logger.h"

int old_user(struct user **users, const char *client_ip, unsigned short client_port, const char *hostname) {
    for (int i = 0; i < MAX_USERS; i++) {
        if (users[i] != NULL) { // Ensure the user slot is in use
            if (strncmp(users[i]->ip, client_ip, INET_ADDRSTRLEN) == 0 && users[i]->port == client_port && strncmp(users[i]->hostname, hostname, NI_MAXHOST) == 0) {
                return i; // Match found, return index
            }
        }
    }
    return -1; // No match found
}

void list_users(struct user **users, size_t user_count) {
    // printf("user_count inside list_user: %d\n", user_count);
    
    /* sorting based on port numbers */
    qsort(users, user_count, sizeof(struct user *), compare_users);

    size_t live_users = 0;

    for (int i = 0; i < MAX_USERS; i++) {
        if (users[i] != NULL && users[i]->status == ONLINE) {
            live_users++;
        }
    }

    if (!live_users) {
        cse4589_print_and_log("[%s:ERROR]\n", "LIST");
        cse4589_print_and_log("[%s:END]\n", "LIST");
        return;
    }

    size_t index;
    int count = 0;
    if (live_users > 0) {
        cse4589_print_and_log("[%s:SUCCESS]\n", "LIST");
        // printf("user count is %d\n", user_count);
        for (index = 0; index < MAX_USERS; index++) {
            if(users[index] != NULL && users[index]->status == ONLINE) {
                count++;
                cse4589_print_and_log("%-5d%-35s%-20s%-8d\n", count, users[index]->hostname, users[index]->ip, users[index]->port);
                live_users--;
                if (!live_users) {
                    break;
                }
            }
        }
        cse4589_print_and_log("[%s:END]\n", "LIST");
    }
}

int clear_blist(struct user *client) {
    size_t count = 0;
    for (size_t i = 0; i < BACKLOG_BUFFER_SIZE; i++) {
        if (client != NULL && client->backlog_list[i] != NULL) {
            free(client->backlog_list[i]);
            client->backlog_list[i] = NULL;
            count++;
        }
    }

    if (count == client->backlog_count) {
        return count;
    } else {
        return count;
    }
}

int add_msg_blist(char *send_ip, char *message, struct user *receiver, int is_broadcast) {
    // printf("Inside add_msg_blist\n");

    for(int i = 0;i < BACKLOG_BUFFER_SIZE; i++) {
        if (receiver != NULL && receiver->backlog_list[i] == NULL) {
	        // printf("location %d of backlog list has NULL\n",i);
            receiver->backlog_list[i] = (struct backlog_msg *)malloc(sizeof(struct backlog_msg));

            strncpy(receiver->backlog_list[i]->sender_ip, send_ip, INET_ADDRSTRLEN);
            strncpy(receiver->backlog_list[i]->message, message, 257);
            receiver->backlog_list[i]->is_broadcast = is_broadcast;
            receiver->backlog_count++;
	        // printf("successfully returning 0 -> success from add_msg_blist\n");
            return 0;
        } 
    }

    return -1;
}

int block (const char *ip_address, struct user* client) {
    
    /* Check to see if the client has already blocked the IP address */
    for (size_t i = 0; i < MAX_LIVE_USERS; i++) {
        if (client->blocked_list[i] != NULL && strncmp(client->blocked_list[i], ip_address, INET_ADDRSTRLEN) == 0) {
            return 0;
        }
    }

    for (size_t i = 0; i < MAX_LIVE_USERS; i++) {
        if (client->blocked_list[i] == NULL) {
            client->blocked_list[i] = (char *) malloc (sizeof(char) * INET_ADDRSTRLEN);
            if (client->blocked_list[i] == NULL) {
                fprintf(stderr, "allocation for blocked list string failed.\n");
            }
            memset ((void *) client->blocked_list[i], '\0', sizeof(char) * INET_ADDRSTRLEN);
            strncpy(client->blocked_list[i], ip_address, INET_ADDRSTRLEN);
            
            return 0;
        }
    }

    return 1;
}

int unblock (const char *ip_address, struct user* client) {
    if (!client) {
        return -1;
    }
    for (size_t i = 0; i < MAX_LIVE_USERS; i++) {
        if (client->blocked_list[i] != NULL && strncmp(client->blocked_list[i], ip_address, INET_ADDRSTRLEN) == 0) {
            free(client->blocked_list[i]);
            client->blocked_list[i] = NULL;
            return 0;
        }
    }
    return 0;
}