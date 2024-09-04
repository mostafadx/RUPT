#define _XOPEN_SOURCE 700
#include <netdb.h>

#include "helper.h"

void init_buffer(RingBuffer *rb){
    atomic_init(&rb->head, 0);
    atomic_init(&rb->tail, 0);
}

bool enqueue(RingBuffer *rb, Task *task)
{
    uint32_t head = atomic_load(&rb->head);
    uint32_t next_head = (head + 1) % BUFFER_SIZE;

    // Check if the buffer is full
    if (next_head == atomic_load(&rb->tail)) {
        return false; // Buffer full
    }

    rb->buffer[head] = *task;
    atomic_store(&rb->head, next_head);
    return true;
}

bool dequeue(RingBuffer *rb, Task *task)
{
    uint32_t tail = atomic_load(&rb->tail);

    // Check if the buffer is empty
    if (tail == atomic_load(&rb->head)) {
        return false; // Buffer empty
    }

    *task = rb->buffer[tail];
    atomic_store(&rb->tail, (tail + 1) % BUFFER_SIZE);
    return true;
}

int get_addr(const char *dst, struct sockaddr * addr)
{
	int ret = 0;
	struct addrinfo *res;
	ret = getaddrinfo(dst, NULL, NULL, &res);
	if (ret) {
		rupt_error("getaddrinfo failed - invalid hostname or IP address\n");
		return ret;
	}

	memcpy(addr, res->ai_addr, sizeof(struct sockaddr_in));
	freeaddrinfo(res);
	return ret;
}
