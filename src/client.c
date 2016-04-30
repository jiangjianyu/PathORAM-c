//
// Created by jyjia on 2016/4/30.
//

#include "client.h"

int get_random_dummy(_bool valid_bits[], int offsets[]);

int gen_reverse_lexicographic(int g);

int read_path(int pos, int address, unsigned char data[], client_ctx *ctx) {
    int i, j, pos_run, found;
    unsigned char *socket_buf = molloc(ORAM_SOCKET_BUFFER);
    crypt_ctx *cpt_ctx;
    socket_ctx *sock_ctx = (socket_ctx *)socket_buf;
    socket_get_metadata *sock_meta = (socket_get_metadata *)sock_ctx->buf;
    socket_get_metadata_r *sock_meta_r = (socket_get_metadata_r *)sock_ctx->buf;
    socket_read_block *sock_block = (socket_read_block *)sock_ctx->buf;
    socket_read_block_r *sock_block_r = (socket_read_block_r *)sock_ctx->buf;
    oram_bucket_encrypted_metadata metadata[ORAM_TREE_DEPTH];
    sock_ctx->type = SOCKET_GET_META;
    sock_meta->pos = pos;
    sendto(ctx->socket, socket_buf, ORAM_SOCKET_META_SIZE, 0, (struct sockaddr *)&ctx->server_addr, ctx->addrlen);
    recvfrom(ctx->socket, socket_buf, ORAM_SOCKET_BUFFER, 0, NULL, NULL);
    for (i = 0, pos_run = pos; ; pos_run >>= 1, ++i) {
        decrypt_message_default(metadata[i].encrypt_metadata, sock_meta_r->metadata[i].encrypt_metadata, ORAM_CRYPT_META_SIZE);
        metadata[i].read_counter = sock_meta_r->metadata[i].read_counter;
        memcpy(metadata[i].valid_bits, sock_meta_r->metadata[i].valid_bits, sizeof(metadata[0].valid_bits));
        if (pos_run == 0)
            break;
    }
    sock_ctx->type = SOCKET_READ_BLOCK;
    sock_block->pos = pos;
    for (i = 0, found = 0, pos_run = pos; ; pos_run >>= 1, ++i) {
        oram_bucket_metadata *de_meta = (oram_bucket_metadata *)&metadata[i].encrypt_metadata;
        if (found == 1)
            sock_block->offsets[i] = get_random_dummy(metadata[i]->valid_bits, de_meta->offset);
        else {
            for (j = 0; j < ORAM_BUCKET_REAL; j++) {
                if (de_meta->address[j] == address && metadata[j].valid_bits[j] == 1) {
                    sock_block->offsets[i] = de_meta->offset[j];
                    found = 1;
                    cpt_ctx = &de_meta->encrypt_par;
                    break;
                }
            }
        }
        if (pos_run == 0)
            break;
    }
    sendto(ctx->socket, socket_buf, ORAM_SOCKET_BLOCK_SIZE, 0, (struct sockaddr *)&ctx->server_addr, ctx->addrlen);
    recvfrom(ctx->socket, socket_buf, ORAM_SOCKET_BLOCK_SIZE_R, 0, NULL, NULL);
    decrypt_message_gen(data, sock_block_r->data, ORAM_CRYPT_DATA_SIZE, cpt_ctx);
}

void access(int address, oram_access_op op, unsigned char data[], client_ctx *ctx) {
    int position_new, position, data_in;
    unsigned char *read_data = malloc(ORAM_BLOCK_SIZE);
    stash_block *block;
    position_new = get_random(ctx->oram_size);
    position = ctx->position_map[address];
    data_in = read_path(position, address, read_data);
    if (data_in == 0)
        block = find_remove_by_address(ctx->stash, address);
    else {
        block = malloc(sizeof(stash_block));
        block->address = address;
    }
    if (op == ORAM_ACCESS_READ)
        memcpy(data, block->data, ORAM_BLOCK_SIZE);
    else
        memcpy(block->data, ORAM_BLOCK_SIZE);
    add_to_stash(ctx->stash, block);
    ctx->round = (++ctx->round) % ORAM_RESHUFFLE_RATE;
    if (ctx->round == 0)
        evit_path(ctx);
}

void evit_path(client_ctx *ctx) {
    unsigned char *socket_buf = malloc(ORAM_SOCKET_BUFFER);
    int pos_run, i;
    socket_ctx *sock_ctx = (socket_ctx *)socket_buf;
    socket_read_bucket *sock_read = (socket_read_bucket *)sock_ctx->buf;
    socket_read_bucket_r *sock_read_r = (socket_read_bucket_r *)sock_ctx->buf;
    socket_write_bucket *sock_write = (socket_write_bucket *)sock_ctx->buf;
    oram_bucket_metadata *meta = malloc(sizeof(oram_bucket_metadata));
    unsigned char data[ORAM_BLOCK_SIZE];
    int pos = gen_reverse_lexicographic(ctx->eviction_g++);
    sock_ctx->type = SOCKET_READ_BUCKET;
    for (pos_run = pos; ;pos_run >>= 1) {
        sock_read->bucket_id = pos_run;
        sendto(ctx->socket, socket_buf, ORAM_SOCKET_READ_SIZE, 0, (struct sockaddr *)&ctx->server_addr, ctx->addrlen);
        recvfrom(ctx->socket, socket_buf, ORAM_SOCKET_BUFFER, 0, NULL, NULL);
        decrypt_message_default((unsigned char *)meta, sock_read_r->bucket.encrypt_metadata, ORAM_CRYPT_META_SIZE);
        for (i = 0;i < ORAM_BUCKET_REAL;i++) {
            if (sock_read_r->bucket.valid_bits[meta->offset[i]] == 1 && meta->address)
        }
        if (pos_run == 0)
            break;
    }
}

void early_reshuffle(int bucket_id);

int client_init(client_ctx *ctx, int size_bucket) {
    int address_size = size_bucket * ORAM_BUCKET_REAL;
    ctx->oram_size = size_bucket;
    ctx->position_map = malloc(sizeof(int) * ctx->oram_size);
    ctx->stash = malloc(sizeof(client_stash));
    bzero(ctx->stash, sizeof(client_stash));
    bzero(ctx->position_map, sizeof(int) * ctx->oram_size);
}