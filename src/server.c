#include <pthread.h>

#include "server.h"
#include "helper.h"

#define MAX_RECV_SIZE 256
#define MESSAGE_SIZE 64
#define NUM_OF_PAGES 512
#define NUM_SERVER_THREADS 1

static void *server_thread(void *);
static int rupt_setup_pd_cq_qp( struct rdma_cm_id *);
static int rupt_setup_mr(void);
static int rupt_setup_wr(void);
static int rupt_accept(struct rdma_cm_id *, struct rdma_event_channel *);
static int rupt_setup_server_threads(rupt_process_message process_message);

void * server_thread(void *data){
    int ret;
    struct ibv_wc wc;
    rupt_process_message process_message = (rupt_process_message) data;

    while(1){
        ret = ibv_poll_cq(server_cb->cq, 1, &wc);
        if (ret){
            process_message((char *)server_cb->pinned_memory + wc.wr_id*MESSAGE_SIZE, MESSAGE_SIZE);

            ret = ibv_post_recv(server_cb->qp, &server_cb->recv_wrs[wc.wr_id], NULL);
            if (ret)
                rupt_error("ibv_post_recv failed, ret=%d\n", ret);
        }
    }

    return NULL;
}

static int rupt_setup_pd_cq_qp( struct rdma_cm_id *cm_id){
    int ret = 0;
    struct ibv_qp_init_attr qp_attr = { };

    /* setup pd */
    {
        server_cb->pd = ibv_alloc_pd(server_cb->cm_id->verbs);
        if (!server_cb->pd){
            rupt_error( "ibv_alloc_pd failed\n");
            ret = -1;
            goto out;
        }
    }

    /* setup cq */
    {
        server_cb->cq = ibv_create_cq(cm_id->verbs, 512, NULL, NULL, 0); 
        if (!server_cb->cq){
            rupt_error( "ibv_create_cq failed\n");
            ret = -1;
            goto out;
        }
    }

    /* setup qp */
    {
        qp_attr.cap.max_send_wr = 512;
        qp_attr.cap.max_recv_wr = 512;
        qp_attr.cap.max_inline_data = 220;
        qp_attr.cap.max_send_sge = 1;
        qp_attr.cap.max_recv_sge = 1;
        qp_attr.send_cq = server_cb->cq;
        qp_attr.recv_cq = server_cb->cq;
        qp_attr.qp_type = IBV_QPT_RC;

        ret = rdma_create_qp(cm_id, server_cb->pd, &qp_attr); 
        if (ret){
            rupt_error("rdma_create_qp failed ret=%d\n", ret);
            goto out;
        }
        server_cb->qp = cm_id->qp;
    }

out:
    return ret;    
}

static int rupt_setup_mr(void)
{
    int ret = 0;

    server_cb->pinned_memory = calloc(NUM_OF_PAGES, 4096);
    if (!server_cb->pinned_memory){
        rupt_error("failed to allocate pinned memory\n");
        ret = -ENOMEM;
        goto out;
    }

    server_cb->mr = ibv_reg_mr(server_cb->pd, server_cb->pinned_memory, 512*4096, 
        IBV_ACCESS_LOCAL_WRITE | 
        IBV_ACCESS_REMOTE_READ | 
        IBV_ACCESS_REMOTE_WRITE); 

    if (!server_cb->mr){
        rupt_error("ibv_reg_mr failed\n");
        ret = -ENOMEM;
        goto out;
    }

out:
    return ret;
}

int rupt_setup_wr(void)
{
    int i;
    uintptr_t offset = 0;

    for(i = 0; i < MAX_RECV_SIZE; i++){
        server_cb->sges[i].addr = (uintptr_t) server_cb->pinned_memory + offset;
        server_cb->sges[i].length = MESSAGE_SIZE;
        server_cb->sges[i].lkey = server_cb->mr->lkey;

        server_cb->recv_wrs[i].next = NULL;
        server_cb->recv_wrs[i].sg_list = &server_cb->sges[i]; 
        server_cb->recv_wrs[i].num_sge = 1;
        server_cb->recv_wrs[i].wr_id = i;

        offset = offset + (uintptr_t) MAX_RECV_SIZE; 
    }

    return 0;

}

int rupt_accept(struct rdma_cm_id *cm_id, struct rdma_event_channel *cm_channel)
{
    int ret = 0;
    struct rdma_cm_event *event; 
    struct rdma_conn_param conn_param = { };

    /* accept */
    {
        conn_param.responder_resources = 1;
        conn_param.private_data = (void *) &server_cb->mr->rkey;
        conn_param.private_data_len = sizeof(server_cb->mr->rkey);

        ret = rdma_accept(cm_id, &conn_param); 
        if (ret) {
            rupt_error("rupt_accept failed, ret=%d\n", ret);
            goto out;
        }
    }

    /* get connection established event */
    {
        ret = rdma_get_cm_event(cm_channel, &event);
        if (ret) {
            rupt_error( "rdma_get_cm_event failed, ret=%d\n",ret);
            goto out;
        }

        if (event->event != RDMA_CM_EVENT_ESTABLISHED){
            ret = -1;
            goto out;
        }

        rdma_ack_cm_event(event);
    }

out:
    return ret;
}

static int rupt_setup_server_threads(rupt_process_message process_message){
    pthread_t threads[NUM_SERVER_THREADS];
    int ret;
    long t;

    for(t = 0; t < NUM_SERVER_THREADS; t++) {
        ret = pthread_create(&threads[t], NULL, server_thread, (void*)process_message);
        if (ret) {
            rupt_error("pthread_create failed, ret=%d\n", ret);
            goto out;
        }
    }

out:
    return ret;
}

int rupt_init_server(const char * addr, const uint16_t port, rupt_process_message process_message){
    int i, ret = 0;
    struct rdma_cm_event *event; 

    /* init cb */
    {
        server_cb = malloc(sizeof(struct rupt_server_handler));
        server_cb->recv_wrs = calloc(MAX_RECV_SIZE,sizeof(struct ibv_recv_wr));
        server_cb->sges     = calloc(MAX_RECV_SIZE,sizeof(struct ibv_sge));
    }   

    /* create cm channel */
    {
        server_cb->cm_channel = rdma_create_event_channel();
        if (!server_cb->cm_channel) 
        {
            rupt_error("creating the event channel\n");
            ret = -1;
            goto out;
        }
    }

    /* create id */
    {
        ret = rdma_create_id(server_cb->cm_channel, &server_cb->listen_id, NULL, RDMA_PS_TCP); 
        if (ret) {
            rupt_error("creating the id\n");
            goto out;
        }
    }

    /* bind addr & listen */
    {
        ret = get_addr(addr, (struct sockaddr*) &server_cb->sin);
        if (ret){
            rupt_error("Invalid IP\n");
            goto out;           
        }
        server_cb->sin.sin_port = port;

        ret = rdma_bind_addr(server_cb->listen_id, (struct sockaddr *) &server_cb->sin);
        if (ret) {
            rupt_error("rdma_bind_addr failed, ret=%d\n",ret);
            goto out;
        }        

        ret = rdma_listen(server_cb->listen_id, 3);
        if (ret){
            rupt_error("rdma_listen failed, ret=%d",ret);
            goto out;
        }

        DEBUG_LOG("Server is listening on port=%d and address=%s\n",port, addr);
    }

    /* get connection request event */
    {
        ret = rdma_get_cm_event(server_cb->cm_channel, &event);
        if (ret){
            rupt_error( "error get the event ret=%d\n",ret);
            goto out;
        }
        if (event->event != RDMA_CM_EVENT_CONNECT_REQUEST){
            rupt_error( "error could not get RDMA_CM_EVENT_CONNECT_REQUEST event\n");
            goto out;
        }

        server_cb->cm_id = event->id;
        rdma_ack_cm_event(event);
    }   

    /* setup pd, cq, and qp */
    {
        ret = rupt_setup_pd_cq_qp(server_cb->cm_id);
        if (ret){
            rupt_error("rupt_setup_pd_cq_qp failed, ret=%d\n", ret);
            goto out;
        }
    }

    /* setup memory region */   
    {
        ret = rupt_setup_mr();
        if (ret){
            rupt_error("rupt_setup_mr failed, ret=%d\n", ret);
            goto out;
        }
    }

    /* setup work requests */
    {
        ret = rupt_setup_wr();
        if (ret){
            rupt_error("rupt_setup_wr failed, ret=%d\n", ret);
            goto out;
        }
    }

    /* post recieve work requests */
    {    
        for(i = 0; i < MAX_RECV_SIZE;i++){
            ret = ibv_post_recv(server_cb->qp, &server_cb->recv_wrs[i], NULL);
            if(ret){
                rupt_error("ibv_post_recv failed, ret=%d\n",ret);
                goto out;
            }
        }
    }

    /* accept client */
    {    
        ret = rupt_accept(server_cb->cm_id, server_cb->cm_channel);
        if (ret){
            rupt_error("rupt_accept failed ret=%d\n", ret);
            goto out;
        }
    }

    /* create threads */
    {
        ret = rupt_setup_server_threads(process_message);
        if (ret){
            rupt_error("rupt_setup_server_threads failed, ret=%d\n", ret);
            goto out;
        }
    }
out:
    return ret;
}