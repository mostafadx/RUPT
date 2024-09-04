#include <stdatomic.h>
#include <stdbool.h>
#include <stdint.h>
#include <netinet/in.h>	
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define rupt_error(msg, args...) do {\
	fprintf(stderr, "%s : %d : ERROR : "msg, __FILE__, __LINE__, ## args);\
}while(0);

/* process queue */

#define BUFFER_SIZE 1024

typedef struct {
    void *data;
} Task;

typedef struct {
    Task buffer[BUFFER_SIZE];
    atomic_uint head;
    atomic_uint tail;
} RingBuffer;

void init_buffer(RingBuffer *rb);
bool enqueue(RingBuffer *rb, Task *task);
bool dequeue(RingBuffer *rb, Task *task);

/* process queue */

/* process ip */

int get_addr(const char *, struct sockaddr *);

/* process ip */