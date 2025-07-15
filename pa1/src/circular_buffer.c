#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "circular_buffer.h"

#define BUFFER_SIZE 2088      /* Size of circular buffer (enough for two maximum packets) */
#define TASK_SIZE 16         /* Fixed task size */

struct CircularBuffer {
    char data[BUFFER_SIZE];
    int head;
    int tail;
    int count;
};

CircularBuffer* create_buffer(void) {
    CircularBuffer *cb = malloc(sizeof(CircularBuffer));
    if (cb) {
        cb->head = 0;
        cb->tail = 0;
        cb->count = 0;
    }
    return cb;
}

void free_buffer(CircularBuffer *cb) {
    if (cb)
        free(cb);
}

int buffer_push(CircularBuffer *cb, char *src, int len) {
    if (cb->count + len > BUFFER_SIZE)
        return -1;
    for (int i = 0; i < len; i++) {
        cb->data[cb->head] = src[i];
        cb->head = (cb->head + 1) % BUFFER_SIZE;
    }
    cb->count += len;
    return len;
}

int buffer_pop_packet(CircularBuffer *cb, char *packet, int len) {
    if (cb->count < 1)
        return 0;

    size_t msg_len = 0;
    int msg_len_idx = cb->tail;
    size_t num_bytes = 0;

    while (isdigit(cb->data[msg_len_idx])) {
        msg_len = msg_len * 10 + (cb->data[msg_len_idx] - '0');
        msg_len_idx = (msg_len_idx + 1) % BUFFER_SIZE;
        num_bytes++;
    }

    int full_packet_size = num_bytes + TASK_SIZE + msg_len + 1;
    if (len < get_max_packet_size() || full_packet_size > get_max_packet_size())
        return -1;

    if (cb->count < full_packet_size)
        return 0;
    
    for (int i = 0; i < full_packet_size; i++) {
        packet[i] = cb->data[cb->tail];
        cb->tail = (cb->tail + 1) % BUFFER_SIZE;
    }
    cb->count -= full_packet_size;
    
    return full_packet_size;
}
