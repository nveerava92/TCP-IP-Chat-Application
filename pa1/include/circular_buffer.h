#ifndef CIRCULAR_BUFFER_H
#define CIRCULAR_BUFFER_H

/* The data structure to use for packet buffering */
typedef struct CircularBuffer CircularBuffer;

/*
 * create_buffer:
 *   Allocates and initializes a CircularBuffer instance.
 *
 * Returns a pointer to a newly allocated CircularBuffer, or NULL on failure.
 */
CircularBuffer* create_buffer(void);

/*
 * free_buffer:
 *   Frees the memory associated with a CircularBuffer instance.
 *
 * Parameters:
 *   - cb: Pointer to the CircularBuffer instance to free.
 */
void free_buffer(CircularBuffer *cb);

/*
 * get_max_packet_size:
 * 
 * Returns:
 *   The maximum size of a single packet that can be stored in the circular buffer.
 */
static inline int get_max_packet_size() {
    return 1044;
}

/*
 * buffer_push:
 *   Pushes 'len' bytes from 'src' into the circular buffer.
 *
 * Parameters:
 *   - cb: Pointer to the circular buffer.
 *   - src: Pointer to the data to be pushed.
 *   - len: Number of bytes to push.
 *
 * Returns:
 *   The number of bytes pushed on success, or -1 if there is insufficient space.
 */
int buffer_push(CircularBuffer *cb, char *src, int len);

/*
 * buffer_pop_packet:
 *   Attempts to extract a complete packet from the circular buffer into 'packet'.
 *   'packet' must be atmost get_max_packet_size() bytes in size.
 *   A complete packet is defined as:
 *       message length (in serial format) + TASK_SIZE bytes (command) + message (in serial format) + null terminator
 *
 * Parameters:
 *   - cb: Pointer to the circular buffer.
 *   - packet: Pointer to a buffer provided by the caller to store the extracted packet.
 *   - len: size of the 'packet' buffer (must be large enough to hold at least one packet).
 *
 * Returns:
 *   The total packet size if a complete packet is available,
 *   0 if a complete packet is not available,
 *   or -1 if an error occurs (including when 'packet' is not large enough).
 */
int buffer_pop_packet(CircularBuffer *cb, char *packet, int len);

#endif /* CIRCULAR_BUFFER_H */