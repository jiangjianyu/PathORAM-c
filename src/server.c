//
// Created by jyjia on 2016/4/29.
//

#include <math.h>
#include <unistd.h>
#include <fcntl.h>
#include <semaphore.h>
#include <pthread.h>
#include "server.h"
#include "log.h"
#include "utlist.h"

void add_to_evict(oram_evict_queue *queue, int bucket_id) {
    //Top Cache
    if (bucket_id < ORAM_TOP_CACHE_SIZE)
        return;
    oram_evict_block *evict_block = NULL;
    oram_evict_list_block *evict_list_block;
    int exist = 1;
    pthread_mutex_lock(&queue->queue_mutex);
    HASH_FIND_INT(queue->hash_queue, &bucket_id, evict_block);
    if (!evict_block) {
        evict_block = malloc(sizeof(oram_evict_block));
        evict_block->count = 0;
        evict_block->bucket_id = bucket_id;
        HASH_ADD_INT(queue->hash_queue, bucket_id, evict_block);
        exist = 0;
    }
    evict_block->count++;
    evict_list_block = malloc(sizeof(oram_evict_list_block));
    evict_list_block->next_l = NULL;
    evict_list_block->evict_block = evict_block;
    LL_APPEND(queue->list_queue, evict_list_block);
    pthread_mutex_unlock(&queue->queue_mutex);
    //Only signal when new bucket is evict
    if (!exist)
        sem_post(&queue->queue_semaphore);
}

oram_bucket* get_bucket(int bucket_id, server_ctx *sv_ctx) {
    storage_ctx *ctx = sv_ctx->sto_ctx;
    if (ctx->bucket_list[bucket_id] == 0) {
        ctx->bucket_list[bucket_id] = read_bucket_from_file(bucket_id);
        add_to_evict(sv_ctx->evict_queue, bucket_id);
    }
    return ctx->bucket_list[bucket_id];
}

void prefetch_bucket(int bucket_id, server_ctx *sv_ctx) {
    storage_ctx *ctx = sv_ctx->sto_ctx;
    if (ctx->bucket_list[bucket_id] == 0) {
        ctx->bucket_list[bucket_id] = read_bucket_from_file(bucket_id);
    }
    add_to_evict(sv_ctx->evict_queue, bucket_id);
}

void read_bucket_path(int pos, oram_bucket *bucket_list, server_ctx *ctx) {
    log_normal("Read Path %d", pos);
    int pos_run, i;
    oram_bucket  *bucket;
    for (pos_run = pos, i = 0; ;i++, pos_run = (pos_run - 1) >> 1) {
        bucket = get_bucket(pos_run, ctx);
        memcpy(&bucket_list[i], bucket, sizeof(oram_bucket));
        if (pos_run == 0)
            break;
    }
}

void write_bucket_path(int pos, oram_bucket *bucket_list, server_ctx *ctx) {
    log_normal("Write Path %d", pos);
    int pos_run, i;
    oram_bucket *bucket;
    for (pos_run = pos, i = 0; ;i++, pos_run = (pos_run - 1) >> 1) {
        bucket = get_bucket(pos_run, ctx);
        memcpy(bucket, &bucket_list[i], sizeof(oram_bucket));
        if (pos_run == 0)
            break;
    }
}

void read_bucket(int bucket_id, socket_read_bucket_r *read_bucket_ctx, server_ctx *ctx) {
    //TODO Check Valid Bits To Decrease Bandwidth
    log_normal("REQUEST->Read Bucket %d", bucket_id);
    oram_bucket *bucket = get_bucket(bucket_id, ctx);
    memcpy(&read_bucket_ctx->bucket, bucket, sizeof(oram_bucket));
    read_bucket_ctx->bucket_id = bucket_id;
}

void write_bucket(int bucket_id, socket_write_bucket *sock_write, server_ctx *ctx) {
    oram_bucket *bucket = &sock_write->bucket;
    log_normal("REQUEST->Write Bucket %d", bucket_id);
    oram_bucket *bucket_sto = get_bucket(bucket_id, ctx);
    memcpy(bucket_sto, bucket, sizeof(oram_bucket));
    bucket_sto->read_counter = 0;
    memset(bucket_sto->valid_bits, 1, sizeof(bucket_sto->valid_bits));
}

void get_metadata(int pos, socket_get_metadata_r *meta_ctx, server_ctx *ctx) {
    log_normal("REQUEST->Get Meta, POS:%d", pos);
    meta_ctx->pos = pos;
    oram_bucket *bucket;
    int i = 0;
    for (; ; pos = (pos - 1) >> 1, ++i) {
        bucket = get_bucket(pos, ctx);
        memcpy(meta_ctx->metadata[i].encrypt_metadata, bucket->encrypt_metadata, ORAM_CRYPT_META_SIZE);
        meta_ctx->metadata[i].read_counter = bucket->read_counter;
        memcpy(meta_ctx->metadata[i].valid_bits, bucket->valid_bits, sizeof(bucket->valid_bits));
        if (pos == 0)
            break;
    }
}

void read_block(int pos, socket_read_block *read, socket_read_block_r *read_block_ctx, server_ctx *ctx) {
    int *offsets = read->offsets;
    log_normal("REQUEST->Read Block, POS:%d", pos);
    int i = 0, j, pos_run = pos;
    oram_bucket *bucket;
    unsigned char return_block[ORAM_CRYPT_DATA_SIZE];
    bzero(return_block, ORAM_CRYPT_DATA_SIZE);
    for (; ; pos_run = (pos_run - 1) >> 1, ++i) {
        // NO Memcpy here to save time
        bucket = get_bucket(pos_run, ctx);
        bucket->valid_bits[offsets[i]] = 0;
        bucket->read_counter++;
        //TODO convert char to int to decrease xor
        for (j = 0;j < ORAM_CRYPT_DATA_SIZE;j++) {
            return_block[j] ^= bucket->data[offsets[i]][j];
        }
        memcpy(read_block_ctx->nonce[i], bucket->data[offsets[i]], ORAM_CRYPT_NONCE_LEN);
        if (pos_run == 0)
            break;
    }
    read_block_ctx->pos = pos;
    memcpy(read_block_ctx->data, return_block, ORAM_CRYPT_DATA_SIZE);
}

void free_server(server_ctx *ctx) {
    if (ctx->sto_ctx->size == 0)
        return;
    oram_evict_block *evict_block = ctx->evict_queue->hash_queue;
    oram_evict_list_block *evict_list_block = ctx->evict_queue->list_queue;
    for (;evict_block != NULL;evict_block = evict_block->hh.next) {
        free(evict_block);
    }
    for (;evict_list_block != NULL;evict_list_block = evict_list_block->next_l) {
        free(evict_list_block);
    }
    ctx->evict_queue->list_queue = NULL;
    ctx->evict_queue->hash_queue = NULL;
    free_storage(ctx->sto_ctx);
}

int server_create(int size, int max_mem, server_ctx *ctx, char key[]) {
    storage_ctx *sto_ctx = ctx->sto_ctx;
    log_sys("REQUEST->Init Server, Size:%d buckets", size);
    int i = 0;
    sto_ctx->size = size;
    sto_ctx->oram_tree_height = log(size + 1) / log(2) + 1;
    sto_ctx->mem_counter = 0;
    sto_ctx->mem_max = max_mem;
    memcpy(sto_ctx->storage_key, key, ORAM_STORAGE_KEY_LEN);
    sto_ctx->bucket_list = calloc(size, sizeof(oram_bucket *));
    for (; i <= size; i++) {
        sto_ctx->bucket_list[i] = new_bucket();
        sto_ctx->mem_counter++;
        add_to_evict(ctx->evict_queue, i);
    }
    log_sys("REQUEST FINISH->Init Server, Size:%d buckets", size);
    return 0;
}

void mem_check(int pos, server_ctx *sv_ctx) {
    storage_ctx *ctx = sv_ctx->sto_ctx;
    if (pos < 0 || ctx->size <= 0)
        return;
    int pos_run;
    for (pos_run = pos;pos_run != 0;pos_run = (pos_run - 1) >> 1) {
        prefetch_bucket(pos_run, sv_ctx);
    }
}

void init_queue(oram_server_queue *queue) {
    queue->queue_list = NULL;
    pthread_mutex_init(&queue->queue_mutex, NULL);
    sem_init(&queue->queue_semaphore, 0, 0);
}

void add_to_server_queue(oram_server_queue_block *block, oram_server_queue *queue) {
    pthread_mutex_lock(&queue->queue_mutex);
    LL_APPEND(queue->queue_list, block);
    pthread_mutex_unlock(&queue->queue_mutex);
    sem_post(&queue->queue_semaphore);
}

void * func_pre(void *args) {
    server_ctx *ctx = (server_ctx *)args;
    ssize_t r;
    int send_len;
    oram_server_queue_block *queue_block;
    unsigned char *buff = malloc(ORAM_SOCKET_BUFFER(ORAM_TREE_DEPTH));
    unsigned char *buff_r = malloc(ORAM_SOCKET_BUFFER(ORAM_TREE_DEPTH));
    socket_ctx *sock_ctx = (socket_ctx *)buff;
    socket_ctx *sock_ctx_r = (socket_ctx *)buff_r;

    socket_read_bucket *read_bucket_ctx = (socket_read_bucket *) sock_ctx->buf;
    socket_write_bucket *write_bucket_ctx = (socket_write_bucket *) sock_ctx->buf;
    socket_read_block *read_block_ctx = (socket_read_block *) sock_ctx->buf;
    socket_get_metadata *get_metadata_ctx = (socket_get_metadata *) sock_ctx->buf;

    socket_path_header *path_header_ctx = (socket_path_header *) sock_ctx->buf;
    socket_path_header *path_header_ctx_r = (socket_path_header *) sock_ctx_r->buf;
    socket_write_bucket_r *write_bucket_ctx_r = (socket_write_bucket_r *) sock_ctx_r->buf;

    while (ctx->running) {
        sem_wait(&ctx->pre_queue->queue_semaphore);
        pthread_mutex_lock(&ctx->pre_queue->queue_mutex);
        queue_block = ctx->pre_queue->queue_list;
        if (queue_block == NULL) {
            log_normal("error reading queue");
            pthread_mutex_unlock(&ctx->pre_queue->queue_mutex);
            continue;
        }
        LL_DELETE(ctx->pre_queue->queue_list, queue_block);
        pthread_mutex_unlock(&ctx->pre_queue->queue_mutex);
        pthread_cond_init(&queue_block->cond, NULL);
        while (1) {
//            bzero(buff, ORAM_SOCKET_BUFFER);
//            bzero(buff_r, ORAM_SOCKET_BUFFER);
            r = recv(queue_block->sock, buff, ORAM_SOCKET_BUFFER_MAX, 0);

            queue_block->data_ready = 0;
            if (r <= 0) {
                log_normal("User Disconnected");
                break;
            }
            queue_block->type = sock_ctx->type;
            switch (sock_ctx->type) {
                case SOCKET_READ_BUCKET:
                    if (r != ORAM_SOCKET_READ_SIZE)
                        sock_recv_add(queue_block->sock, buff, r, ORAM_SOCKET_READ_SIZE);
                    sock_ctx_r->type = SOCKET_READ_BUCKET;
                    send_len = ORAM_SOCKET_READ_SIZE_R;
                    queue_block->pos = read_bucket_ctx->bucket_id;
                    break;
                case SOCKET_WRITE_BUCKET:
                    if (r != ORAM_SOCKET_WRITE_SIZE)
                        sock_recv_add(queue_block->sock, buff, r, ORAM_SOCKET_WRITE_SIZE);
                    sock_ctx_r->type = SOCKET_WRITE_BUCKET;
                    write_bucket_ctx_r->bucket_id = write_bucket_ctx->bucket_id;
                    write_bucket_ctx_r->type = SOCKET_RESPONSE_SUCCESS;
                    send_len = ORAM_SOCKET_WRITE_SIZE_R;
                    queue_block->pos = write_bucket_ctx->bucket_id;
                    break;
                case SOCKET_READ_PATH:
                    if (r != ORAM_SOCKET_PATH_SIZE)
                        sock_recv_add(queue_block->sock, buff, r, ORAM_SOCKET_PATH_SIZE);
                    sock_ctx_r->type = SOCKET_READ_PATH;
                    queue_block->pos = path_header_ctx->pos;
                    send_len = ORAM_SOCKET_PATH_SIZE + sizeof(oram_bucket) * path_header_ctx->len;
                    path_header_ctx_r->pos = path_header_ctx->pos;
                    path_header_ctx_r->len = path_header_ctx->len;
                    break;
                case SOCKET_WRITE_PATH:
                    if (r != ORAM_SOCKET_PATH_SIZE)
                        r = sock_recv_add(queue_block->sock, buff, r, ORAM_SOCKET_PATH_SIZE);
                    queue_block->pos = path_header_ctx->pos;
                    sock_recv_add(queue_block->sock, buff, r, sizeof(oram_bucket) * path_header_ctx->len + ORAM_SOCKET_PATH_SIZE);
                    sock_ctx_r->type = SOCKET_WRITE_PATH;
                    send_len = ORAM_SOCKET_PATH_SIZE;
                    path_header_ctx_r->pos = -1;
                    path_header_ctx_r->len = path_header_ctx->len;
                    break;
                case SOCKET_GET_META:
                    if (r != ORAM_SOCKET_META_SIZE)
                        sock_recv_add(queue_block->sock, buff, r, ORAM_SOCKET_META_SIZE);
                    sock_ctx_r->type = SOCKET_GET_META;
                    send_len = ORAM_SOCKET_META_SIZE_R(ctx->sto_ctx->oram_tree_height);
                    queue_block->pos = get_metadata_ctx->pos;
                    break;
                case SOCKET_READ_BLOCK:
                    if (r != ORAM_SOCKET_BLOCK_SIZE(ctx->sto_ctx->oram_tree_height))
                        sock_recv_add(queue_block->sock, buff, r,
                                      ORAM_SOCKET_BLOCK_SIZE(ctx->sto_ctx->oram_tree_height));
                    sock_ctx_r->type = SOCKET_READ_BLOCK;
                    send_len = ORAM_SOCKET_BLOCK_SIZE_R(ctx->sto_ctx->oram_tree_height);
                    queue_block->pos = read_block_ctx->pos;
                    break;
                case SOCKET_INIT:
                    if (r != ORAM_SOCKET_INIT_SIZE)
                        sock_recv_add(queue_block->sock, buff, r, ORAM_SOCKET_INIT_SIZE);
                    sock_ctx_r->type = SOCKET_INIT;
                    send_len = ORAM_SOCKET_INIT_SIZE_R;
                    queue_block->pos = -1;
                    break;
                default:
                    continue;
            }
            queue_block->buff = sock_ctx->buf;
            queue_block->buff_r = sock_ctx_r->buf;
            mem_check(queue_block->pos, ctx);
            add_to_server_queue(queue_block, ctx->main_queue);
//            pthread_mutex_lock(&ctx->main_queue->queue_mutex);
            while (queue_block->data_ready == 0)
                continue;
//                pthread_cond_wait(&queue_block->cond, &ctx->main_queue->queue_mutex);
//            pthread_mutex_unlock(&ctx->main_queue->queue_mutex);
            sock_standard_send(queue_block->sock, buff_r, send_len);
        }
    }
    return 0;
}

//TODO lock_function
void * func_main(void *args) {
    server_ctx *ctx = (server_ctx *)args;
    oram_server_queue_block *queue_block;
    int status;
    socket_init *init_ctx;
    socket_init_r *init_ctx_r;

    while (ctx->running) {
        sem_wait(&ctx->main_queue->queue_semaphore);
        pthread_mutex_lock(&ctx->main_queue->queue_mutex);
        queue_block = ctx->main_queue->queue_list;
        if (queue_block == NULL) {
            log_sys("error in main queue");
            pthread_mutex_unlock(&ctx->main_queue->queue_mutex);
            continue;
        }
        LL_DELETE(ctx->main_queue->queue_list, queue_block);
        pthread_mutex_unlock(&ctx->main_queue->queue_mutex);
        switch (queue_block->type) {
            case SOCKET_INIT:
                init_ctx = (socket_init *) queue_block->buff;
                init_ctx_r = (socket_init_r *) queue_block->buff_r;
                oram_init_op op_main = init_ctx->op & 0x03;
                oram_init_op op_re = init_ctx->op & 0x04;
                if (op_main == SOCKET_OP_CREATE) {
                    if (ctx->sto_ctx->size != 0 && op_re != SOCKET_OP_REINIT) {
                        status = -1;
                        sprintf(init_ctx_r->err_msg, "Already init.");
                    }
                    else {
                        free_server(ctx);
                        status = server_create(init_ctx->size, ctx->args->max_mem, ctx, init_ctx->storage_key);
                    }
                }
                else if (op_main == SOCKET_OP_LOAD) {
                    if (strcmp(ctx->sto_ctx->storage_key, init_ctx->storage_key) == 0)
                        status = 0;
                    else {
                        if (ctx->sto_ctx->size != 0 && op_re != SOCKET_OP_REINIT) {
                            status = -1;
                            sprintf(init_ctx_r->err_msg, "Already init.");
                        }
                        else {
                            free_server(ctx);
                            if ((status = server_load(ctx, init_ctx->storage_key)) < 0)
                                sprintf(init_ctx_r->err_msg, "Key mismatch");
                        }
                    }
                }
                else if (op_main == SOCKET_OP_SAVE) {
                    status = server_save(ctx->sto_ctx);
                    ctx->running = 0;
                }
                else
                    status = -1;

                if (status == 0)
                    init_ctx_r->status = SOCKET_RESPONSE_SUCCESS;
                else
                    init_ctx_r->status = SOCKET_RESPONSE_FAIL;
                break;
            case SOCKET_READ_BUCKET:
                read_bucket(queue_block->pos, (socket_read_bucket_r *)queue_block->buff_r, ctx);
                break;
            case SOCKET_WRITE_BUCKET:
                write_bucket(queue_block->pos, (socket_write_bucket *)queue_block->buff, ctx);
                break;
            case SOCKET_GET_META:
                get_metadata(queue_block->pos, (socket_get_metadata_r *)queue_block->buff_r, ctx);
                break;
            case SOCKET_READ_PATH:
                read_bucket_path(queue_block->pos, (oram_bucket *)(queue_block->buff_r + sizeof(socket_path_header)), ctx);
                break;
            case SOCKET_WRITE_PATH:
                write_bucket_path(queue_block->pos, (oram_bucket *)(queue_block->buff + sizeof(socket_path_header)), ctx);
                break;
            case SOCKET_READ_BLOCK:
                read_block(queue_block->pos, (socket_read_block *)queue_block->buff,
                           (socket_read_block_r *)queue_block->buff_r, ctx);
                break;
            default:
                break;
        }
        queue_block->data_ready = 1;
    }
    return 0;
}

void * func_evict(void *args) {
    server_ctx *ctx = (server_ctx *)args;
    int found = 0;
    oram_evict_list_block *list_block;
    while (ctx->running) {
        sem_wait(&ctx->evict_queue->queue_semaphore);
        if (ctx->sto_ctx->mem_counter < ctx->sto_ctx->mem_max)
            continue;
        pthread_mutex_lock(&ctx->evict_queue->queue_mutex);
        found = 0;
        while (!found) {
            list_block = ctx->evict_queue->list_queue;
            if (list_block == NULL) {
                err("error in queue");
                pthread_mutex_unlock(&ctx->evict_queue->queue_mutex);
                break;
            }

            if (list_block->evict_block->count > 1) {
                list_block->evict_block->count--;
            } else {
                write_bucket_to_file(list_block->evict_block->bucket_id, ctx->sto_ctx, 0);
                HASH_DEL(ctx->evict_queue->hash_queue, list_block->evict_block);
                free(list_block->evict_block);
                found = 1;
            }
            LL_DELETE(ctx->evict_queue->list_queue, list_block);
            free(list_block);
        }
        pthread_mutex_unlock(&ctx->evict_queue->queue_mutex);
    }
    return 0;
}

//TODO USE LESS SHARE BUFFER, MORE HEAP
void server_run(oram_args_t *args, server_ctx *sv_ctx) {
    int i;
    storage_ctx *sto_ctx = malloc(sizeof(storage_ctx));
    pthread_t pre_thread[args->worker];
    pthread_t main_thread, evict_thread;
    bzero(sv_ctx, sizeof(server_ctx));
    bzero(sto_ctx, sizeof(storage_ctx));
    oram_server_queue_block *queue_block;
    sv_ctx->sto_ctx = sto_ctx;
    sto_ctx->size = 0;
    sv_ctx->args = args;
    if (sock_init_byhost(&sv_ctx->server_addr, &sv_ctx->addrlen, &sv_ctx->socket_listen, args->host, args->port, 1) < 0)
        return;
    sv_ctx->running = 1;

    //Init worker
    sv_ctx->pre_queue = malloc(sizeof(oram_server_queue));
    sv_ctx->main_queue = malloc(sizeof(oram_server_queue));
    sv_ctx->evict_queue = malloc(sizeof(oram_evict_queue));
    init_queue(sv_ctx->pre_queue);
    init_queue(sv_ctx->main_queue);
    sv_ctx->evict_queue->hash_queue = NULL;
    sv_ctx->evict_queue->list_queue = NULL;
    pthread_mutex_init(&sv_ctx->evict_queue->queue_mutex, NULL);
    sem_init(&sv_ctx->evict_queue->queue_semaphore, 0, 0);
    for (i = 0;i < args->worker;i++)
        pthread_create(&pre_thread[i], NULL, func_pre, (void *)sv_ctx);
    pthread_create(&main_thread, NULL, func_main, (void *)sv_ctx);
    pthread_create(&evict_thread, NULL, func_evict, (void *)sv_ctx);
    struct sockaddr_in tem_sockaddr;
    socklen_t tem_socklen;
    while (sv_ctx->running == 1) {
        queue_block = (oram_server_queue_block *)malloc(sizeof(oram_server_queue_block));
        queue_block->sock = accept(sv_ctx->socket_listen, (struct sockaddr *)&tem_sockaddr, &tem_socklen);
        log_normal("New User Connected.");
        add_to_server_queue(queue_block, sv_ctx->pre_queue);
    }
}

int server_save(storage_ctx *ctx) {
    log_f("server saved");
    int fd = open(ORAM_FILE_META_FORMAT, O_WRONLY | O_CREAT, S_IRUSR|S_IWUSR);
    if (fd < 0) {
        errf("Store server to file fail.");
        return 0;
    }
    write(fd, ctx, sizeof(storage_ctx));
    close(fd);
    flush_buckets(ctx);
    return 0;
}

int server_load(server_ctx *sv_ctx, char key[]) {
    storage_ctx *ctx = malloc(sizeof(storage_ctx));
    bzero(ctx, sizeof(storage_ctx));
    log_f("server loaded");
    int fd = open(ORAM_FILE_META_FORMAT, O_RDONLY);
    int i;
    read(fd, ctx, sizeof(storage_ctx));
    ctx->bucket_list = calloc(ctx->size, sizeof(oram_bucket *));
    ctx->mem_counter = 0;
    if (strcmp(key, ctx->storage_key) != 0) {
        free(ctx);
        return -1;
    }
    free(sv_ctx->sto_ctx);
    sv_ctx->sto_ctx = ctx;
    //Do not read too many buckets into memory
    for (i = 0;i < ctx->mem_max;i++) {
        ctx->bucket_list[i] = read_bucket_from_file(i);
        ctx->mem_counter++;
    }
    close(fd);
    return 0;
}

void server_stop(server_ctx *sv_ctx) {
    sv_ctx->running = 0;
    close(sv_ctx->socket_listen);
}