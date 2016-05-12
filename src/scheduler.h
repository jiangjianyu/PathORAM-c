//
// Created by maxxie on 16-5-12.
//

#ifndef PATHORAM_SCHEDULER_H
#define PATHORAM_SCHEDULER_H

#include "args.h"
#include "client.h"
#include <semaphore.h>

typedef struct oram_request_queue_block{
    int sock;
    int address;
    oram_access_op op;
    unsigned char data[ORAM_BLOCK_SIZE];
    UT_hash_handle hh;
    struct oram_request_queue_block *next_l;
} oram_request_queue_block;

typedef struct {
    oram_request_queue_block *queue_list;
    oram_request_queue_block *queue_hash;
    pthread_mutex_t queue_mutex;
    sem_t queue_semaphore;
} oram_request_queue;

int listen_accept(oram_client_args *args,client_ctx *ctx);

int add_to_queue(int sock, oram_request_queue *queue);

int init_worker();

//Worker function
void * worker_func(void *args);

#endif //PATHORAM_SCHEDULER_H
