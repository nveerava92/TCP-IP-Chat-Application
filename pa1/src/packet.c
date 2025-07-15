#include "packet.h"

struct Packet {
    char *data;
    size_t data_len;
    char *command;
    size_t message_len;
};

size_t make_packet(char *cmd, size_t cmd_len, char *msg, size_t msg_len, Packet **created_pkt) {
    if (!created_pkt || !cmd)
        return 0;

    if (cmd_len > (COMMAND_SIZE - 1) || msg_len > (MAX_MSG_SIZE - 1)) {
        *created_pkt = NULL;
        return 0;
    }

    size_t total_len = 4 + COMMAND_SIZE + (msg_len + 1);

    Packet *pkt = (Packet *)malloc(sizeof(Packet));
    if (!pkt) {
        *created_pkt = NULL;
        return 0;
    }
    pkt->data = (char *)malloc(total_len);
    if (!pkt->data) {
        free(pkt);
        *created_pkt = NULL;
        return 0;
    }
    memset(pkt->data, '\0', total_len);

    // pkt->data_len = total_len;
    sprintf(pkt->data, "%zu", msg_len);
    pkt->command = pkt->data + strlen(pkt->data);
    memcpy(pkt->data + strlen(pkt->data), cmd, cmd_len);

    if (msg != NULL && msg_len > 0) {
        memcpy(pkt->command + COMMAND_SIZE, msg, msg_len);
    }

    // pkt->command = pkt->data + 1;
    pkt->message_len = msg_len;
    pkt->data_len = (pkt->command - pkt->data) + COMMAND_SIZE + msg_len + 1;
    
    *created_pkt = pkt;
    return total_len;
}

void free_packet(Packet *pkt) {
    if (pkt) {
        if (pkt->data)
            free(pkt->data);
        free(pkt);
    }
}

ssize_t send_packet(int socket_fd, Packet *outgoing_pkt) {
    if (!outgoing_pkt || !outgoing_pkt->data)
        return -1;
    size_t total = outgoing_pkt->data_len, sent = 0;

    while (sent < total) {
        ssize_t n = send(socket_fd, outgoing_pkt->data + sent, total - sent, 0);
        if (n <= 0)
            return -1;
        sent += n;
    }
    return sent;
}

ssize_t receive_packets(int socket_fd, Packet ***pkt_buf, CircularBuffer *cb) {
    if (!pkt_buf || !cb)
        return -1;

    size_t capacity = 10, count = 0;
    Packet **packets = (Packet **)malloc(capacity * sizeof(Packet *));
    if (!packets) {
        *pkt_buf = NULL;
        return -1;
    }

    char recv_buffer[MAX_PACKET_SIZE];
    char temp_packet[MAX_PACKET_SIZE];
    while (1) {
        int bytes_received = recv(socket_fd, recv_buffer, sizeof(recv_buffer), 0);
        if (bytes_received <= 0) {
            if (bytes_received == 0) {
                if (count) break;
                goto unexpected_shutdown;
            }
            if (errno == EWOULDBLOCK || errno == EAGAIN) {
                if (count) break;
                fprintf(stderr, "Unable to receive data from socket %d\n", socket_fd);
                goto error;
            }
        }

        int pushed_data_len = 0;
        while ((pushed_data_len = buffer_push(cb, recv_buffer + pushed_data_len, bytes_received - pushed_data_len)) <= bytes_received) {
            while (1) {
                int packet_size = buffer_pop_packet(cb, temp_packet, MAX_PACKET_SIZE);
                if (packet_size == 0) {
                    break;
                }

                Packet *new_pkt = (Packet *)malloc(sizeof(Packet));
                if (!new_pkt)
                    goto error;
                new_pkt->data = (char *)malloc(packet_size);
                if (!new_pkt->data) {
                    free(new_pkt);
                    goto error;
                }
                memset(new_pkt->data, '\0', packet_size);

                memcpy(new_pkt->data, temp_packet, packet_size);
                new_pkt->data_len = packet_size;
                
                size_t msg_len = 0;
                int msg_len_idx = 0;

                while (isdigit(new_pkt->data[msg_len_idx])) {
                    msg_len = msg_len * 10 + (new_pkt->data[msg_len_idx] - '0');
                    msg_len_idx++;
                }

                new_pkt->command = new_pkt->data + (size_t)msg_len_idx;
                new_pkt->message_len = msg_len;
                
                if (count >= capacity) {
                    capacity *= 2;
                    Packet **temp = (Packet **)realloc(packets, capacity * sizeof(Packet *));
                    if (!temp) {
                        free_packet(new_pkt);
                        goto error;
                    }
                    packets = temp;
                }
                packets[count++] = new_pkt;
            }

            if (pushed_data_len == bytes_received) {
                break;
            }
        }
        
    }

    if (count == 0) {
        free(packets);
        *pkt_buf = NULL;
    } else {
        Packet **temp = realloc(packets, count * sizeof(Packet *));
        if (temp)
            packets = temp;
        *pkt_buf = packets;
    }

    return count;

unexpected_shutdown:
    free(packets);
    *pkt_buf = NULL;
    return -2;

error:
    for (size_t i = 0; i < count; i++)
        free_packet(packets[i]);

    free(packets);
    *pkt_buf = NULL;
    return -1;
}

ssize_t get_length(Packet *pkt) {
    if (!pkt)
        return -1;
    return pkt->data_len;
}

ssize_t get_command(Packet *pkt, char **cmd) {
    if (!pkt || !cmd)
        return -1;
    *cmd = pkt->command;
    return (ssize_t)strnlen(pkt->command, COMMAND_SIZE);
}

ssize_t get_message(Packet *pkt, char **msg) {
    if (!pkt || !msg)
        return -1;
    char *msg_ptr = pkt->command + COMMAND_SIZE;
    *msg = msg_ptr;
    return strlen(msg_ptr);
}
