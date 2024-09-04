#pragma once 

#include <stdlib.h>
#include <stdint.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <getopt.h>
#include <pthread.h>
#include <netdb.h>
#include <netinet/in.h>	
#include <arpa/inet.h>
#include <sys/socket.h>
#include <infiniband/verbs.h>
#include <rdma/rdma_cma.h> 

#include "rupt.h"

struct rupt_client_handler 
{
    struct sockaddr_in sin;
    struct rdma_cm_id *cm_id;
    struct rdma_event_channel *cm_channel;

    struct ibv_pd           *pd; 
    struct ibv_cq           *cq;  
    struct ibv_qp           *qp;

    void *pinned_memory;
    struct ibv_mr *mr; 

    struct ibv_sge sge;
    struct ibv_recv_wr recv_wr;

    uint64_t remote_addr;
    uint32_t remote_key;

} *client_cb;