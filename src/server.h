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

struct rupt_server_handler
{                                                               
    struct sockaddr_in sin;
    struct rdma_cm_id           *listen_id; 
    struct rdma_cm_id           *cm_id; 
    struct rdma_cm_event        *event;  
    struct rdma_event_channel   *cm_channel;       
    struct ibv_pd           *pd; 
    struct ibv_cq           *cq;  
    struct ibv_qp           *qp;
    void *pinned_memory;
    struct ibv_mr *mr; 
    struct ibv_recv_wr *recv_wrs;
    struct ibv_sge *sges;
} *server_cb; /* object of rkit server handler */

