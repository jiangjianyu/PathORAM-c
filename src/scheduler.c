//
// Created by maxxie on 16-5-12.
//

#include <pthread.h>
#include "scheduler.h"
#include "client.h"


int listen_accept(oram_client_args *args, client_ctx *ctx) {
    int i;
    if (sock_init(&ctx->server_addr, &ctx->addrlen,&ctx->server_sock, args->host, args->port, 1) < 0)
        return -1;

    //Init oram_request_queue
    ctx->queue = malloc(sizeof(oram_request_queue));
    ctx->queue->queue_hash = NULL;
    ctx->queue->queue_list = NULL;
    sem_init(&ctx->queue->queue_semaphore, 0, 0);
    pthread_mutex_init(&ctx->queue->queue_mutex, NULL);

    ctx->worker_id = calloc(args->worker, sizeof(pthread_t));
    for (i = 0;i < args->worker;i++) {
        pthread_create(&ctx->worker_id[i], NULL, worker_func, (void *)ctx);
    }

    struct sockaddr_in tem_addr;
    socklen_t tem_addrlen;
    while (ctx->running) {
        int sock_recv = accept(ctx->server_sock, (struct sockaddr *)&tem_addr, &tem_addrlen);
        add_to_queue(sock_recv, ctx->queue);
    }
}

void * worker_func(void *args) {
    client_ctx *ctx = (client_ctx *)args;
    oram_request_queue_block *queue_block;
    while (ctx->running) {
        sem_wait(&ctx->queue->queue_semaphore);
        pthread_mutex_lock(&ctx->queue->queue_mutex);
        queue_block = ctx->queue->queue_list;
        if (queue_block != NULL)
            LL_DELETE(ctx->queue->queue_list, queue_block);
        pthread_mutex_unlock(&ctx->queue->queue_mutex);
        int server_addr = queue_block->address % ctx->oram_size;
        int server_id = queue_block->address / ctx->oram_size;
        oblivious_access(server_addr, queue_block->op, queue_block->data, ctx);
    }
    //process access address

}

int add_to_queue(int sock, oram_request_queue *queue) {
    socket_access access_buffer;
    int r = recv(sock, &access_buffer, ORAM_SOCKET_ACCESS_SIZE, 0);
    oram_request_queue_block *block = malloc(sizeof(oram_request_queue_block));
    block->address = access_buffer.address;
    block->op = access_buffer.op;
    if (block->op == ORAM_ACCESS_WRITE)
        memcpy(block->data, access_buffer.data, ORAM_BLOCK_SIZE);

    pthread_mutex_lock(&queue->queue_mutex);
    HASH_ADD_INT(queue->queue_hash, address, block);
    LL_PREPEND(queue->queue_list, block);
    pthread_mutex_unlock(&queue->queue_mutex);
    sem_post(&queue->queue_semaphore);
}