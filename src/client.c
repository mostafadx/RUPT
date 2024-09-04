#include <stdatomic.h>
#include <stdbool.h>

#include "client.h"
#include "helper.h"

#define NUM_OF_PAGES 512

static atomic_int message_counter = ATOMIC_VAR_INIT(0);

static int rupt_setup_mr(void);
static int rupt_setup_pd_cq_qp( struct rdma_cm_id *);
static int rupt_connect_to_server();

static int rupt_setup_pd_cq_qp( struct rdma_cm_id *cm_id){
    int ret = 0;
    struct ibv_qp_init_attr qp_attr = { };

    /* setup pd */
    {
        client_cb->pd = ibv_alloc_pd(client_cb->cm_id->verbs);
        if (!client_cb->pd){
            rupt_error( "ibv_alloc_pd failed\n");
            ret = -1;
            goto out;
        }
    }

    /* setup cq */
    {
        client_cb->cq = ibv_create_cq(cm_id->verbs, 512, NULL, NULL, 0); 
        if (!client_cb->cq){
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
        qp_attr.send_cq = client_cb->cq;
        qp_attr.recv_cq = client_cb->cq;
        qp_attr.qp_type = IBV_QPT_RC;

        ret = rdma_create_qp(cm_id, client_cb->pd, &qp_attr); 
        if (ret){
            rupt_error("rdma_create_qp failed ret=%d\n", ret);
            goto out;
        }
        client_cb->qp = cm_id->qp;
    }

out:
    return ret;    
}

static int rupt_setup_mr(void)
{
    int ret = 0;

    client_cb->pinned_memory = calloc(NUM_OF_PAGES, 4096);
    if (!client_cb->pinned_memory){
        rupt_error("failed to allocate pinned memory\n");
        ret = -ENOMEM;
        goto out;
    }

    client_cb->mr = ibv_reg_mr(client_cb->pd, client_cb->pinned_memory, 512*4096, 
        IBV_ACCESS_LOCAL_WRITE | 
        IBV_ACCESS_REMOTE_READ | 
        IBV_ACCESS_REMOTE_WRITE); 

    if (!client_cb->mr){
        rupt_error("ibv_reg_mr failed\n");
        ret = -ENOMEM;
        goto out;
    }

out:
    return ret;
}

static int rupt_connect_to_server()
{
    int ret = 0;
    struct rdma_conn_param conn_param = { };
    struct rdma_cm_event *event;

    conn_param.initiator_depth = 1;
    conn_param.retry_count     = 7;

    ret = rdma_connect(client_cb->cm_id, &conn_param);
    if (ret){
        rupt_error("rdma_connect failed, ret=%d\n", ret);
        goto out;
    }

    ret = rdma_get_cm_event(client_cb->cm_channel, &event);
    if (ret){
        rupt_error("rdma_get_cm_event failed, ret=%d\n", ret);
        goto out;
    }

    if (event->event != RDMA_CM_EVENT_ESTABLISHED){
      rupt_error( "error could not get RDMA_CM_EVENT_ESTABLISHED event\n");
      ret = -1;
      goto out;
    }    

    client_cb->remote_key = *((uint32_t *)event->param.conn.private_data);

out:
    return ret;
}

int rupt_init_client(const char * addr, uint16_t port){

    int ret = 0;
    struct rdma_cm_event *event;
    
    /* init cb */
    {
        client_cb = malloc(sizeof(struct rupt_client_handler));
    }

    /* create event channel */
    {
        client_cb->cm_channel = rdma_create_event_channel(); 
        if (!client_cb->cm_channel) {
            rupt_error("rdma_create_event_channel failed\n");
            ret = -1;
            goto out;
        }
    }

    /* create id */
    {
        ret = rdma_create_id(client_cb->cm_channel, &client_cb->cm_id, NULL, RDMA_PS_TCP);
        if (ret)
        {
            rupt_error("rdma_create_id failed, ret=%d\n", ret);
            goto out;
        }
    }

    /* bind to server*/
    {

        ret = get_addr(addr, (struct sockaddr*) &client_cb->sin);
        if (ret){
            rupt_error("get_addr failed, ret=%d\n", ret);
            goto out;
        }
        client_cb->sin.sin_port = port;

        ret = rdma_resolve_addr(client_cb->cm_id, NULL, (struct sockaddr *) &client_cb->sin, 5000);
        if (ret){
            rupt_error("rdma_resolve_addr failed, ret=%d\n", ret);
            goto out;
        }

        ret = rdma_get_cm_event(client_cb->cm_channel, &event);
        if (ret){
            rupt_error( "rdma_get_cm_event failed, ret=%d\n",ret);
            goto out; 
        }

        if (event->event != RDMA_CM_EVENT_ADDR_RESOLVED){
            rupt_error( "error could not get RDMA_CM_EVENT_ADDR_RESOLVED event\n");
            ret = -1;
            goto out;
        }  

        rdma_ack_cm_event(event);

        ret = rdma_resolve_route(client_cb->cm_id, 5000);
        if (ret){
            rupt_error( "rdma_resolve_route failed, ret=%d\n",ret);
            goto out;
        }            

        ret = rdma_get_cm_event(client_cb->cm_channel, &event);
        if (ret){
            rupt_error( "rdma_get_cm_event failed, ret=%d\n",ret);
            goto out;
        }        

        if (event->event != RDMA_CM_EVENT_ROUTE_RESOLVED)
        {
            rupt_error( "error could not get RDMA_CM_EVENT_ROUTE_RESOLVED event\n");
            ret = -1;
            goto out;
        }

        rdma_ack_cm_event(event);
    }

    /* setup pd cq qp*/
    {
        ret = rupt_setup_pd_cq_qp(client_cb->cm_id);
        if (ret){
            rupt_error("rupt_setup_pd_cq_qp failed, ret=%d\n", ret);
            goto out;
        }
    }

    /* create memory region */
    {
        ret = rupt_setup_mr();
        if (ret){
            rupt_error("rupt_setup_mr failed, ret=%d\n", ret);
            goto out;
        }
    }

    /* connect to server */
    {
        ret = rupt_connect_to_server();
        if (ret){
            rupt_error("rupt_connect_client failed, ret=%d\n", ret);
            goto out;
        }
    }

    DEBUG_LOG("Client successfully connected to port=%d addr=%s\n",port, addr);

out:
    return ret;
}

int rupt_send_message(const char * message, unsigned long size)
{
    int counter, ret = 0;
    struct ibv_send_wr wr;
    struct ibv_sge sge;
    struct ibv_wc wc;

    counter = atomic_fetch_add(&message_counter, 1) + 1;

    /* setup work request */
    {
        sge.addr = (uintptr_t) message;
        sge.length = size;
        sge.lkey = client_cb->mr->lkey;

        wr.next = NULL;
        wr.sg_list = &sge;
        wr.num_sge = 1;
        wr.opcode = IBV_WR_SEND;
        wr.send_flags = IBV_SEND_INLINE;

        /* Selective Signaling */
        if (counter%100)
            wr.send_flags |= IBV_SEND_SIGNALED;
    }

    ret = ibv_post_send(client_cb->qp, &wr, NULL);
    if (ret){
        rupt_error("ibv_post_send failed, ret=%d\n", ret);
        return ret;
    }
    
    /* Selective Signaling */
    if (counter%100)
    {    
        while(!ibv_poll_cq(client_cb->cq, 1, &wc));

        if (wc.status)
            rupt_error("ibv_poll_cq failed, wc.status: %d\n", wc.status);
    }
    return ret;
}

