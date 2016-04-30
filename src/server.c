//
// Created by jyjia on 2016/4/29.
//

#include "server.h"
#include "socket.h"

int read_bucket(int bucket_id, socket_read_bucket_r *read_bucket_ctx, storage_ctx *sto_ctx) {
    oram_bucket *bucket = get_bucket(bucket_id, sto_ctx);
    memcpy(&read_bucket_ctx->bucket, bucket, sizeof(oram_bucket));
    read_bucket_ctx->bucket_id = bucket_id;
}

int write_bucket(int bucket_id, oram_bucket *bucket, storage_ctx *sto_ctx) {
    oram_bucket *bucket_sto = get_bucket(bucket_id, sto_ctx);
    memcpy(bucket_sto, bucket, sizeof(oram_bucket));
    bucket_sto->read_counter = 0;
    memset(bucket_sto->valid_bits, 1, sizeof(bucket_sto->valid_bits));
}

int get_metadata(int pos, socket_get_metadata_r *meta_ctx, storage_ctx *sto_ctx) {
    meta_ctx->pos = pos;
    int i = 0;
    for (; ; pos >>= 1, ++i) {
        memcpy(&meta_ctx->metadata[i], get_bucket(pos, sto_ctx)->encrypt_metadata, sizeof(ORAM_CRYPT_META_SIZE));
        pos >>= 1;
        if (pos == 0)
            break;
    }
}

int read_block(int pos, int offsets[], socket_read_block_r *read_block_ctx, storage_ctx *sto_ctx) {
    int i = 0;
    oram_bucket *bucket;
    bzero(sto_ctx->return_block, sizeof(ORAM_CRYPT_DATA_SIZE));
    for (; ; pos >>= 1, ++i) {
        // NO Memcpy here to save time
        bucket = get_bucket(pos, sto_ctx);
        bucket->valid_bits[offsets[i]] = 0;
        bucket->read_counter++;
        sto_ctx->return_block ^= bucket->data[offsets[i]];
        if (pos == 0)
            break;
    }
    read_block_ctx->pos = pos;
    memcpy(read_block_ctx->data, sto_ctx->return_block, sizeof(ORAM_CRYPT_DATA_SIZE));
}

int init_server(int size, storage_ctx *sto_ctx) {
    int i = 0;
    for (; i <= size; i++) {
        sto_ctx->bucket_list[i] = malloc(sizeof(oram_bucket));
        sto_ctx->bucket_list[i]->read_counter = 0;
        memset(sto_ctx->bucket_list[i]->valid_bits, 1, sizeof(sto_ctx->bucket_list[i]->valid_bits));
    }
}

int server_run(oram_args_t *args) {
    server_ctx *sv_ctx = malloc(sizeof(server_ctx));
    storage_ctx *sto_ctx = malloc(sizeof(storage_ctx));
    bzero(sv_ctx, sizeof(server_ctx));
    bzero(sto_ctx, sizeof(storage_ctx));
    inet_aton(args->host, &sv_ctx->server_addr.sin_addr);
    sv_ctx->server_addr.sin_port = htons(args->port);
    sv_ctx->server_addr.sin_family = AF_INET;
    sv_ctx->socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    sv_ctx->addrlen = sizeof(sv_ctx->server_addr);
    sv_ctx->buff = malloc(ORAM_SOCKET_BUFFER);
    sv_ctx->buff_r = malloc(ORAM_SOCKET_BUFFER);
    while (sv_ctx->running == 1) {
        bzero(sv_ctx->buff, ORAM_SOCKET_BUFFER);
        bzero(sv_ctx->buff_r, ORAM_SOCKET_BUFFER);
        recvfrom(sv_ctx->socket, sv_ctx->buff, ORAM_SOCKET_BUFFER,
                 0, (struct sockaddr *)&sv_ctx->client_addr,
                 &sv_ctx->addrlen);
        socket_ctx *sock_ctx = (socket_ctx *)sv_ctx->buff;
        socket_ctx *sock_ctx_r = (socket_ctx *)sv_ctx->buff_r;
        switch (sock_ctx->type) {
            case SOCKET_READ_BUCKET: read_bucket(
                        ((socket_read_bucket *)sock_ctx->buf)->bucket_id,
                        (socket_read_bucket_r *)sock_ctx_r->buf, sto_ctx);
                sendto(sv_ctx->socket, sv_ctx->buff_r, ORAM_SOCKET_READ_SIZE,
                       0, (struct sockaddr *)&sv_ctx->client_addr, sv_ctx->addrlen);
                break;
            case SOCKET_WRITE_BUCKET:
                socket_write_bucket *write_ctx = (socket_write_bucket *)sock_ctx->buf;
                write_bucket(write_ctx->bucket_id, &write_ctx->bucket, sto_ctx);
                break;
            case SOCKET_GET_META:
                get_metadata(((socket_get_metadata *)sock_ctx->buf)->pos,
                             (socket_get_metadata_r *)sock_ctx_r->buf, sto_ctx);
                sendto(sv_ctx->socket, sv_ctx->buff_r, ORAM_SOCKET_META_SIZE,
                       0, (struct sockaddr *)&sv_ctx->client_addr, sv_ctx->addrlen);
                break;
            case SOCKET_READ_BLOCK:
                socket_read_block *read_block_ctx = (socket_read_block *)sock_ctx->buf;
                read_block(read_block_ctx->pos, read_block_ctx->offsets, (socket_read_block_r *)sock_ctx_r->buf, sto_ctx);
                sendto(sv_ctx->socket, sv_ctx->buff_r, ORAM_SOCKET_BLOCK_SIZE,
                       0, (struct sockaddr *)&sv_ctx->client_addr, sv_ctx->addrlen);
                break;
            case SOCKET_INIT:
                init_server(((socket_init *)sock_ctx->buf)->size, sto_ctx);
                break;
            default:
                break;
        }
    }
}