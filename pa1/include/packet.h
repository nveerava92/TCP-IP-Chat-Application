#ifndef PACKET_H
#define PACKET_H

#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <errno.h>

#include "circular_buffer.h"

#define COMMAND_SIZE 16
#define MAX_MSG_SIZE 1024
#define MAX_PACKET_SIZE (4 + COMMAND_SIZE + MAX_MSG_SIZE)

/* Opaque Packet structure */
typedef struct Packet Packet;

/*
 * make_packet():
 *   Creates a Packet from the provided command and message.
 *   Allocates memory for the Packet and its internal data.
 *
 *   - cmd: Pointer to the command string.
 *   - cmd_len: Length of the command string. Must not exceed COMMAND_SIZE.
 *   - msg: Pointer to the message string.
 *   - msg_len: Length of the message string. Must not exceed MAX_MSG_SIZE.
 *   - created_pkt: Output parameter. On success, *created_pkt is set to point to the allocated Packet.
 *
 * Returns the total size of the Packet (strlen(itoa(msg_len)) + COMMAND_SIZE + msg_len + 1) on success,
 * or 0 on failure (with *created_pkt set to NULL).
 */
size_t make_packet(char *cmd, size_t cmd_len, char *msg, size_t msg_len, Packet **created_pkt);

/*
 * free_packet():
 *   Frees the memory associated with the Packet.
 */
void free_packet(Packet *pkt);

/*
 * send_packet():
 *   Sends the entire Packet over the given socket.
 *
 * Returns the total number of bytes sent on success, or -1 on error.
 */
ssize_t send_packet(int socket_fd, Packet *outgoing_pkt);

/*
 * receive_packets():
 *   Receives data from the socket, uses the provided CircularBuffer to buffer incoming data,
 *   and extracts complete packets into a dynamically allocated array.
 *
 *   The caller provides a pointer to a Packet pointer array (Packet ***pkt_buf).
 *   On success, *pkt_buf is set to a dynamically allocated array of Packet pointers,
 *   sized exactly to the number of complete packets extracted.
 *   The caller is responsible for freeing each Packet with free_packet() and the array itself.
 *
 *   In case of error during execution, *pkt_buf is set to NULL.
 *
 * Returns the number of complete packets extracted, 0 if no complete packets to be received,
 * or -1 if an error occurs.
 */
ssize_t receive_packets(int socket_fd, Packet ***pkt_buf, CircularBuffer *cb);

/*
 * get_length():
 *   Returns the total length of the Packet (including the length byte, command, and message).
 */
ssize_t get_length(Packet *pkt);

/*
 * get_command():
 *   Retrieves a pointer to the command stored in the Packet.
 *
 *   - cmd: Output parameter. On success, *cmd is set to point to the command string within the Packet.
 *          The caller should not free this pointer; instead, free the entire Packet with free_packet().
 *
 * Returns the length of the command (up to the first NULL or COMMAND_SIZE) on success,
 * or -1 on error.
 */
ssize_t get_command(Packet *pkt, char **cmd);

/*
 * get_message():
 *   Retrieves a pointer to the message stored in the Packet.
 *
 *   - msg: Output parameter. On success, *msg is set to point to the message string within the Packet.
 *          The caller should not free this pointer; instead, free the entire Packet with free_packet().
 *
 * Returns the length of the message on success,
 * or -1 on error.
 */
ssize_t get_message(Packet *pkt, char **msg);

#endif /* PACKET_H */
