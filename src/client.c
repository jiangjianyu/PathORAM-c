//
// Created by jyjia on 2016/4/30.
//

#include "client.h"

int get_random_dummy(_bool valid_bits[], int offsets[]) {
    int i;
    for (i = ORAM_BUCKET_REAL;i < ORAM_BUCKET_SIZE;i++) {
        if (valid_bits[offsets[i]] == 1)
            return offsets[i];
    }
    return 0;
}

int gen_reverse_lexicographic(int g) {
    int reverse_int = 0, i;
    for (i = 0;i < ORAM_TREE_DEPTH;++i) {
        reverse_int <<= 1;
        ret |= num & 1;
        num >>= 1;
    }
    return reverse_int;
}

int read_bucket(client_ctx *ctx ,int bucket_id,
                unsigned char *socket_buf, oram_bucket_metadata *meta) {
    int i;
    socket_ctx *sock_ctx = (socket_ctx *)socket_buf;
    socket_read_bucket *sock_read = (socket_read_bucket *)sock_ctx->buf;
    socket_read_bucket_r *sock_read_r = (socket_read_bucket_r *)sock_ctx->buf;
    sock_read->bucket_id = pos_run;
    sendto(ctx->socket, socket_buf, ORAM_SOCKET_READ_SIZE, 0, (struct sockaddr *)&ctx->server_addr, ctx->addrlen);
    recvfrom(ctx->socket, socket_buf, ORAM_SOCKET_BUFFER, 0, NULL, NULL);
    decrypt_message_default((unsigned char *)meta, sock_read_r->bucket.encrypt_metadata, ORAM_CRYPT_META_SIZE);
    for (i = 0;i < ORAM_BUCKET_REAL;i++) {
        if (sock_read_r->bucket.valid_bits[meta->offset[i]] == 1 && meta->address[meta->offset[i]] != 0) {
            block->address = meta->address[meta->offset[i]];
            block->bucket_id = sock_read_r->bucket_id;
            decrypt_message_gen(block->data, sock_read_r->bucket.data[meta->offset[i]], ORAM_BLOCK_SIZE, meta->encrypt_par);
            add_to_stash(ctx->stash, block);
        }
    }
}

int write_bucket(client_ctx *ctx, int bucket_id, stash_block *stash_list,
                 unsigned char *socket_buf, oram_bucket_metadata *meta) {
    int count,i;
    socket_ctx *sock_ctx = (socket_ctx *)socket_buf;
    socket_write_bucket *sock_write = (socket_write_bucket *)sock_ctx->buf;
    count = find_remove_by_bucket(ctx->stash, bucket_id, ORAM_BUCKET_REAL, stash_list);
    get_random_permutation(ORAM_BUCKET_SIZE, meta->offset);
    gen_crypt_pair(meta->encrypt_par);
    for (i = 0;i < count;i++) {
        encrypt_message_gen(sock_write->bucket.data[meta->offset[i]], stash_list[i].data, ORAM_BLOCK_SIZE, &meta->encrypt_par);
        meta->address[i] = stash_list[i].address;
    }
    for (i = count; i < ORAM_BUCKET_SIZE; i++)
        encrypt_message_gen(sock_write->bucket.data[meta->offset[i]], ctx->blank_data, ORAM_BLOCK_SIZE, &meta->encrypt_par);
    sock_write->bucket.read_counter = 0;
    memset(sock_write->bucket.valid_bits, 1, sizeof(sock_write->bucket.valid_bits));
    encrypt_message_default(sock_write->bucket.encrypt_metadata, (unsigned char *)meta, ORAM_META_SIZE);
    sendto(ctx->socket, socket_buf, SOCKET_WRITE_BUCKET, 0, (struct sockaddr *)&ctx->server_addr, ctx->addrlen);
}

int get_metadata(int pos, unsigned char *socket_buf, oram_bucket_encrypted_metadata metadata[], client_ctx *ctx) {
    int pos_run, i;
    socket_ctx *sock_ctx = (socket_ctx *)socket_buf;
    socket_get_metadata *sock_meta = (socket_get_metadata *)sock_ctx->buf;
    socket_get_metadata_r *sock_meta_r = (socket_get_metadata_r *)sock_ctx->buf;
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
}

int read_block(int pos, unsigned char socket_buf, unsigned char data[], client_ctx *ctx) {
    int found, i, pos_run;
    unsigned char xor_tem[ORAM_CRYPT_DATA_SIZE];
    socket_ctx *sock_ctx = (socket_ctx *)socket_buf;
    socket_read_block *sock_block = (socket_read_block *)sock_ctx->buf;
    socket_read_block_r *sock_block_r = (socket_read_block_r *)sock_ctx->buf;
    sock_ctx->type = SOCKET_READ_BLOCK;
    sock_block->pos = pos;
    oram_bucket_metadata *de_meta;
    crypt_ctx *cpt_ctx;
    for (i = 0, found = 0, pos_run = pos; ; pos_run >>= 1, ++i) {
        de_meta = (oram_bucket_metadata *)&metadata[i].encrypt_metadata;
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
    for (i = 0, pos_run = pos;;pos_run >>= 1, ++i) {
        encrypt_message_gen(xor_tem, ctx->blank_data, ORAM_BLOCK_SIZE, &de_meta->encrypt_par);
        sock_block_r->data ^= xor_tem;
        if (pos_run == 0)
            break;
    }
    decrypt_message_gen(data, sock_block_r->data, ORAM_CRYPT_DATA_SIZE, cpt_ctx);
}

int read_path(int pos, int address, unsigned char data[], client_ctx *ctx) {
    int i, j, pos_run, found;
    unsigned char socket_buf[ORAM_SOCKET_BUFFER];
    oram_bucket_encrypted_metadata metadata[ORAM_TREE_DEPTH];
    get_metadata(pos, socket_buf, metadata, ctx);
    read_block(pos, socket_buf, data, ctx);
    early_reshuffle(pos, metadata, ctx);
}

void access(int address, oram_access_op op, unsigned char data[], client_ctx *ctx) {
    int position_new, position, data_in;
    unsigned char read_data[ORAM_BLOCK_SIZE];
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

void evict_path(client_ctx *ctx) {
    unsigned char socket_buf[ORAM_SOCKET_BUFFER];
    int pos_run;
    socket_ctx *sock_ctx = (socket_ctx *)socket_buf;
    int pos = gen_reverse_lexicographic(ctx->eviction_g);
    ctx->eviction_g = (ctx->eviction_g++) % ORAM_LEAF_SIZE;
    sock_ctx->type = SOCKET_READ_BUCKET;
    //some share buffer
    oram_bucket_metadata meta;
    stash_block block;
    for (pos_run = pos; ;pos_run >>= 1) {
        read_bucket(ctx, pos_run, socket_buf, &meta);
        if (pos_run == 0)
            break;
    }

    sock_ctx->type = SOCKET_WRITE_BUCKET;
    stash_block *stash_list = calloc(ORAM_BUCKET_REAL, sizeof(stash_block));
    for (pos_run = pos; ;pos_run >>= 1) {
        write_bucket(bucket_id, stash_list, &meta);
        if (pos_run == 0)
            break;
    }
}

void early_reshuffle(int pos, oram_bucket_encrypted_metadata metadata[], client_ctx *ctx) {
    int i, pos_run;
    oram_bucket_metadata meta;
    unsigned char socket_buf[ORAM_SOCKET_BUFFER];
    for (i = 0, pos_run = pos;; pos_run >>= 1, ++i) {
        if (metadata[i].read_counter >= ORAM_BUCKET_DUMMY) {
            read_bucket(ctx, pos_run, socket_buf, &meta);
            write_bucket(ctx, pos_run, ctx->stash, socket_buf, &meta);
        }
    }
}

int client_init(client_ctx *ctx, int size_bucket, oram_args_t *args) {
    int address_size = size_bucket * ORAM_BUCKET_REAL;
    ctx->oram_size = size_bucket;
    ctx->position_map = malloc(sizeof(int) * ctx->oram_size);
    ctx->stash = malloc(sizeof(client_stash));
    inet_aton(args->host, &ctx->server_addr.sin_addr);
    ctx->server_addr.sin_port = htons(args->port);
    ctx->server_addr.sin_family = AF_INET;
    ctx->socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    ctx->addrlen = sizeof(ctx->server_addr);
    bzero(ctx->stash, sizeof(client_stash));
    bzero(ctx->position_map, sizeof(int) * ctx->oram_size);
}
