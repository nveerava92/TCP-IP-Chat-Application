#include "logger.h"
#include "global.h"

ssize_t generate_users_string(char **msg, int count, struct user *users[]) {
    char* temp = (char*)malloc(sizeof(char) * MAX_MSG_SIZE);
    if (temp == NULL) {
        fprintf(stderr, "Error allocating memory for users string\n");
    }
    memset(temp, '\0', sizeof(char) * MAX_MSG_SIZE);

    /* This part is simply for debugging */
    int live_users = 0;

    /* Building the string into data_string in the format:
     *  COUNT<US>USER1->IP<RS>USER1->HOSTNAME<RS>USER1->PORT<US>USER2->IP<RS>USER2->HOSTNAME<RS>...
    */
    const char unit_separator[] = {31, 0};
    const char record_separator[] = {30, 0};
    sprintf(temp + strlen(temp), "%d", count);
    strcat(temp, unit_separator);    
    for (int i = 0; i < MAX_USERS; i++) {
        if (users[i] != NULL && users[i]->status == ONLINE) {
            live_users++;
            strcat(temp, users[i]->ip);
            strcat(temp, record_separator);
            strcat(temp, users[i]->hostname);
            strcat(temp, record_separator);
            sprintf(temp + strlen(temp), "%d", users[i]->port);
            strcat(temp, unit_separator);
        }
    }

    if (live_users != count) {
        fprintf(stderr, "Something is wrong with the provided COUNT argument provided with generate_users_string\n");
    } 

    /* Removing the last Unit Separator */
    temp[strlen(temp) - 1] = '\0';

    *msg = temp;
    return strlen(temp);
}

ssize_t parse_users_string(const char *msg, size_t msg_len, user *users[]) {
    ssize_t temp_count = 0;
    ssize_t us_count = 0;
    char *user_data;
    const char unit_separator[] = {31, 0};
    // const char record_separator[] = {30, 0};
    
    char *msg_copy = (char *)malloc(sizeof(char) * MAX_MSG_SIZE);
    if (msg_copy == NULL) {
        fprintf(stderr, "Memeory allocation failed in parse_users_data function\n");
    }
    memset(msg_copy, '\0', sizeof(char) * MAX_MSG_SIZE);
    

    /* Creating a duplicte of the server's message */
    strncpy(msg_copy, msg, msg_len + 1);

    /* Extracting count */
    user_data = strtok(msg_copy, unit_separator);
    if (user_data == NULL) {
        free(msg_copy);
        return -1;
    }
    temp_count = atoi(user_data);

    size_t user_index = 0;

    /* Reinitializing the list before filling it up with users details */
    for (; user_index < MAX_USERS; user_index++) {
        if (users[user_index] != NULL) {
            free(users[user_index]);
        }
        users[user_index] = NULL;
    }
    user_index = 0;

    user_data = strtok(NULL, unit_separator);
    while (user_data != NULL && user_index < MAX_USERS) {
        us_count++;
        users[user_index] = (user *)malloc(sizeof(user));
        if (users[user_index] == NULL) {
            fprintf(stderr, "Memory allocation for user failed in parse_users_data funtion\n");
            free(msg_copy);
            return -1;
        }

        /* Using sscanf to extract ip, hostname, and port from the segment */
        if (sscanf(user_data, "%[^\x1E]%*c%[^\x1E]%*c%hu", users[user_index]->ip, users[user_index]->hostname, &users[user_index]->port) != 3) {
            fprintf(stderr, "Unable to parse users information from the received buffer\n");
            free(msg_copy);
            return -1;
        }
        users[user_index]->status = ONLINE;

        user_data = strtok(NULL, unit_separator);
        user_index++;
    }

    if (us_count == temp_count) {
        free(msg_copy);
        return us_count;
    }

    free(msg_copy);
    return -1;
}

ssize_t generate_backlog_string(char **msg, struct user *client) {
    if (client->backlog_count == 0) {
        return 0;
    }

    char* temp = (char *)malloc(sizeof(char) * MAX_MSG_SIZE);
    if (temp == NULL) {
        perror("Allocating memory for BACKLOG buffer failed");
    }
    memset(temp, '\0', sizeof(char) * MAX_MSG_SIZE);

    /* Building the string into data_string in the format:
     * COUNT<US>USER1->IP<RS>USER1->MSG<US>USER2->IP<RS>USER2->MSG<US>...
    */
    sprintf(temp + strlen(temp), "%zu", client->backlog_count);
    sprintf(temp + strlen(temp), "%c", 31);

    for (int i = 0; i < BACKLOG_BUFFER_SIZE; i++) {
        if (client->backlog_list[i] != NULL) {
            strcat(temp, client->backlog_list[i]->sender_ip);
            sprintf(temp + strlen(temp), "%c", 30);
            strcat(temp, client->backlog_list[i]->message);
            sprintf(temp + strlen(temp), "%c", 31);
        }
    }

    /* Replacing the last delimiter with null terminator */
    temp[strlen(temp) - 1] = '\0';

    *msg = temp;

    /* printf("The function generate_backlog_string executed successfully and is now returning!!\n"); */
    return strlen(temp);
}

ssize_t parse_backlog_string(char *data_string) {
    const char unit_separator[] = {31, 0};
    const char record_separator[] = {30, 0};

    char *rest = data_string;

    /* Extracting the message count */
    char *count_str = strtok_r(rest, unit_separator, &rest);
    int count = atoi(count_str);

    for (int i = 0; i < count; ++i) {
        char *ip = strtok_r(rest, record_separator, &rest);
        if (ip == NULL) {
            break;
        }

        char *msg = strtok_r(rest, unit_separator, &rest);
        if (msg == NULL) {
            /* If we reach here, it means there's an inconsistency in data */
            fprintf(stderr, "Data inconsistency encountered when parsing backlog string. Expected message not found.\n");
            return -1;
        }

        cse4589_print_and_log("[%s:SUCCESS]\n", "RECEIVED");
        cse4589_print_and_log("msg from:%s\n[msg]:%s\n",ip,msg);
        cse4589_print_and_log("[%s:END]\n", "RECEIVED");
    }

    return count;
}

ssize_t generate_msg_string(char **msg, char *send_ip, char *message) {
    char *temp = malloc(sizeof(char) * MAX_MSG_SIZE);
    if (temp == NULL) {
        perror("Allocating memory for BACKLOG buffer failed");
    }
    memset(temp, '\0', sizeof(char) * MAX_MSG_SIZE);

    /* Building the string into data_string in the format:
    *  SENDER_IP<RS>MESSAGE
    */
    strcat(temp, send_ip);
    sprintf(temp + strlen(temp), "%c", (char)30);
    strcat(temp, message);
    sprintf(temp + strlen(temp), "%c", (char)30);


    // Replacing the last delimiter with null terminator
    temp[strlen(temp) - 1] = '\0';

    // printf("The function generate_msg_string is going to return the following string:\n%s\n", data_string);
    // printf("The function generate_message_string executed successfully and is now returning!!\n");

    *msg = temp;
    return strlen(temp);
}

ssize_t parse_msg_string(char *msg, char **dest_ip, char **message, int send) {
    if (!send && dest_ip) {
        return -1;
    }

    if (send) {
        const char record_separator[] = {30, 0};
        char *ip;
        char *out_msg;

        ip = strtok(msg, record_separator);
        if (!ip) {
            fprintf(stderr, "Unable to retrieve destination IP from the packet\n");
            return -1;
        }
        char *temp = (char *)malloc(INET_ADDRSTRLEN);
        if (!temp) {
            fprintf(stderr, "Unable to allocate memory for destination ip string\n");
            return -1;
        }
        memset(temp, '\0', INET_ADDRSTRLEN);

        strncpy(temp, ip, INET_ADDRSTRLEN);
        *dest_ip = temp;

        out_msg = strtok(NULL, record_separator);
        if (!out_msg) {
            fprintf(stderr, "Unable to retrieve user message from the packet\n");
            return -1;
        }
    
        char *temp2 = (char *)malloc(257);
        if (!temp2) {
            fprintf(stderr, "Unable to allocate memory for user message\n");
            return -1;
        }
        memset(temp2, '\0', 257);
    
        strncpy(temp2, out_msg, strlen(out_msg));
        *message = temp2;
    
        return strlen(out_msg);
    }

    char *temp2 = (char *)malloc(257);
    if (!temp2) {
        fprintf(stderr, "Unable to allocate memory for user message\n");
        return -1;
    }
    memset(temp2, '\0', 257);

    strncpy(temp2, msg, strlen(msg));
    *message = temp2;

    return strlen(msg);
}
