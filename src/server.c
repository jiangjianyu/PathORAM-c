//
// Created by jyjia on 2016/4/29.
//

#include <math.h>
#include <unistd.h>
#include <fcntl.h>
#include "server.h"
#include "log.h"

void read_bucket(int bucket_id, socket_read_bucket_r *read_bucket_ctx, storage_ctx *sto_ctx) {
    //TODO Check Valid Bits To Decrease Bandwidth
    log_f("REQUEST->Read Bucket %d", bucket_id);
    oram_bucket *bucket = get_bucket(bucket_id, sto_ctx);
    memcpy(&read_bucket_ctx->bucket, bucket, sizeof(oram_bucket));
    read_bucket_ctx->bucket_id = bucket_id;
}

void write_bucket(int bucket_id, oram_bucket *bucket, storage_ctx *sto_ctx) {
    log_f("REQUEST->Write Bucket %d", bucket_id);
    oram_bucket *bucket_sto = get_bucket(bucket_id, sto_ctx);
    memcpy(bucket_sto, bucket, sizeof(oram_bucket));
    bucket_sto->read_counter = 0;
    memset(bucket_sto->valid_bits, 1, sizeof(bucket_sto->valid_bits));
}

void get_metadata(int pos, socket_get_metadata_r *meta_ctx, storage_ctx *sto_ctx) {
    log_f("REQUEST->Get Meta, POS:%d", pos);
    meta_ctx->pos = pos;
    oram_bucket *bucket;
    int i = 0;
    for (; ; pos = (pos - 1) >> 1, ++i) {
        bucket = get_bucket(pos, sto_ctx);
        memcpy(meta_ctx->metadata[i].encrypt_metadata, bucket->encrypt_metadata, ORAM_CRYPT_META_SIZE);
        meta_ctx->metadata[i].read_counter = bucket->read_counter;
        memcpy(meta_ctx->metadata[i].valid_bits, bucket->valid_bits, sizeof(bucket->valid_bits));
        if (pos == 0)
            break;
    }
}

void read_block(int pos, int offsets[], socket_read_block_r *read_block_ctx, storage_ctx *sto_ctx) {
    log_f("REQUEST->Read Block, POS:%d", pos);
    int i = 0, j, pos_run = pos;
    oram_bucket *bucket;
    unsigned char return_block[ORAM_CRYPT_DATA_SIZE];
    bzero(return_block, ORAM_CRYPT_DATA_SIZE);
    for (; ; pos_run = (pos_run - 1) >> 1, ++i) {
        // NO Memcpy here to save time
        bucket = get_bucket(pos_run, sto_ctx);
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

int server_create(int size, int max_mem, storage_ctx *sto_ctx, char key[]) {
    log_f("REQUEST->Init Server, Size:%d buckets", size);
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
        evict_to_disk(sto_ctx, i);
    }
    return 0;
}

//TODO USE LESS SHARE BUFFER, MORE HEAP
void server_run(oram_args_t *args, server_ctx *sv_ctx) {
    ssize_t r;
    storage_ctx *sto_ctx = malloc(sizeof(storage_ctx));
    bzero(sv_ctx, sizeof(server_ctx));
    bzero(sto_ctx, sizeof(storage_ctx));
    sv_ctx->sto_ctx = sto_ctx;
    if (sock_init_byhost(&sv_ctx->server_addr, &sv_ctx->addrlen, &sv_ctx->socket_listen, args->host, args->port, 1) < 0)
        return;
    sv_ctx->buff = malloc(ORAM_SOCKET_BUFFER);
    sv_ctx->buff_r = malloc(ORAM_SOCKET_BUFFER);
    sv_ctx->running = 1;
    while (sv_ctx->running == 1) {
        bzero(sv_ctx->buff, ORAM_SOCKET_BUFFER);
        bzero(sv_ctx->buff_r, ORAM_SOCKET_BUFFER);
        sv_ctx->socket_data = accept(sv_ctx->socket_listen,
                                     (struct sockaddr *) &sv_ctx->client_addr,
                                     &sv_ctx->addrlen);
        log_f("User Connected");
        while (1) {
            r = recv(sv_ctx->socket_data, sv_ctx->buff, ORAM_SOCKET_BUFFER, 0);
            if (r <= 0) {
                log_f("User Disconnected");
                break;
            }
            socket_ctx *sock_ctx = (socket_ctx *) sv_ctx->buff;
            socket_ctx *sock_ctx_r = (socket_ctx *) sv_ctx->buff_r;

            socket_read_bucket *read_bucket_ctx = (socket_read_bucket *) sock_ctx->buf;
            socket_write_bucket *write_ctx = (socket_write_bucket *) sock_ctx->buf;
            socket_read_block *read_block_ctx = (socket_read_block *) sock_ctx->buf;
            socket_get_metadata *get_metadata_ctx = (socket_get_metadata *) sock_ctx->buf;
            socket_init *init_ctx = (socket_init *) sock_ctx->buf;

            socket_read_block_r *read_block_ctx_r = (socket_read_block_r *) sock_ctx_r->buf;
            socket_read_bucket_r *read_bucket_ctx_r = (socket_read_bucket_r *) sock_ctx_r->buf;
            socket_get_metadata_r *get_metadata_ctx_r = (socket_get_metadata_r *) sock_ctx_r->buf;
            socket_write_bucket_r *write_bucket_ctx_r = (socket_write_bucket_r *) sock_ctx_r->buf;
            socket_init_r *init_ctx_r = (socket_init_r *) sock_ctx_r->buf;
            switch (sock_ctx->type) {
                case SOCKET_READ_BUCKET:
                    if (r != ORAM_SOCKET_READ_SIZE)
                        sock_recv_add(sv_ctx->socket_data, sv_ctx->buff, r, ORAM_SOCKET_READ_SIZE);
                    read_bucket(read_bucket_ctx->bucket_id,
                                read_bucket_ctx_r, sv_ctx->sto_ctx);
                    sock_ctx_r->type = SOCKET_READ_BUCKET;
                    sock_standard_send(sv_ctx->socket_data, sv_ctx->buff_r, ORAM_SOCKET_READ_SIZE_R);
                    break;
                case SOCKET_WRITE_BUCKET:
                    if (r != ORAM_SOCKET_WRITE_SIZE)
                        sock_recv_add(sv_ctx->socket_data, sv_ctx->buff, r, ORAM_SOCKET_WRITE_SIZE);
                    //TODO return
                    sock_ctx_r->type = SOCKET_WRITE_BUCKET;
                    write_bucket(write_ctx->bucket_id, &write_ctx->bucket, sv_ctx->sto_ctx);
                    write_bucket_ctx_r->bucket_id = write_ctx->bucket_id;
                    write_bucket_ctx_r->type = SOCKET_RESPONSE_SUCCESS;
                    sock_standard_send(sv_ctx->socket_data, sv_ctx->buff_r, ORAM_SOCKET_WRITE_SIZE_R);
                    break;
                case SOCKET_GET_META:
                    if (r != ORAM_SOCKET_META_SIZE)
                        sock_recv_add(sv_ctx->socket_data,sv_ctx->buff, r, ORAM_SOCKET_META_SIZE);
                    sock_ctx_r->type = SOCKET_GET_META;
                    get_metadata(get_metadata_ctx->pos, get_metadata_ctx_r, sv_ctx->sto_ctx);
                    sock_standard_send(sv_ctx->socket_data, sv_ctx->buff_r, ORAM_SOCKET_META_SIZE_R(sv_ctx->sto_ctx->oram_tree_height));
                    break;
                case SOCKET_READ_BLOCK:
                    if (r != ORAM_SOCKET_BLOCK_SIZE(sv_ctx->sto_ctx->oram_tree_height))
                        sock_recv_add(sv_ctx->socket_data, sv_ctx->buff, r, ORAM_SOCKET_BLOCK_SIZE(sv_ctx->sto_ctx->oram_tree_height));
                    read_block(read_block_ctx->pos, read_block_ctx->offsets, read_block_ctx_r, sv_ctx->sto_ctx);
                    sock_standard_send(sv_ctx->socket_data, sv_ctx->buff_r, ORAM_SOCKET_BLOCK_SIZE_R(sv_ctx->sto_ctx->oram_tree_height));
                    break;
                case SOCKET_INIT:
                    if (r != ORAM_SOCKET_INIT_SIZE)
                        sock_recv_add(sv_ctx->socket_data, sv_ctx->buff, r, ORAM_SOCKET_INIT_SIZE);

                    int status;
                    oram_init_op op_main = init_ctx->op & 0x03;
                    oram_init_op op_re = init_ctx->op & 0x04;
                    if (op_main == SOCKET_OP_CREATE) {
                        if (sv_ctx->sto_ctx->size != 0 && op_re != SOCKET_OP_REINIT) {
                            status = -1;
                            sprintf(init_ctx_r->err_msg, "Already init.");
                        }
                        else {
                            free_server(sv_ctx->sto_ctx);
                            status = server_create(init_ctx->size, args->max_mem, sv_ctx->sto_ctx, init_ctx->storage_key);
                        }
                    }
                    else if (op_main == SOCKET_OP_LOAD) {
                        if (strcmp(sv_ctx->sto_ctx->storage_key, init_ctx->storage_key) == 0)
                            status = 0;
                        else {
                            if (sv_ctx->sto_ctx->size != 0 && op_re != SOCKET_OP_REINIT) {
                                status = -1;
                                sprintf(init_ctx_r->err_msg, "Already init.");
                            }
                            else {
                                free_server(sv_ctx->sto_ctx);
                                if ((status = server_load(sv_ctx, init_ctx->storage_key)) < 0)
                                    sprintf(init_ctx_r->err_msg, "Key mismatch");
                            }
                        }
                    }
                    else if (op_main == SOCKET_OP_SAVE) {
                        status = server_save(sv_ctx->sto_ctx);
                        sv_ctx->running = 0;
                    }
                    else
                        status = -1;

                    if (status == 0)
                        init_ctx_r->status = SOCKET_RESPONSE_SUCCESS;
                    else
                        init_ctx_r->status = SOCKET_RESPONSE_FAIL;
                    sock_standard_send(sv_ctx->socket_data, sv_ctx->buff_r, ORAM_SOCKET_INIT_SIZE_R);
                    break;
                default:
                    break;
            }
        }
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