//
// Created by jyjia on 2016/4/30.
//


#include "client.h"

int get_random_dummy(_Bool valid_bits[], int offsets[]) {
    int i;
    for (i = ORAM_BUCKET_REAL;i < ORAM_BUCKET_SIZE;i++) {
        if (valid_bits[offsets[i]] == 1)
            return offsets[i];
    }
    logf("error, not enough");
    return 0;
}

int gen_reverse_lexicographic(int g) {
    int reverse_int = 0, i;
    for (i = 0;i < ORAM_TREE_DEPTH;++i) {
        reverse_int <<= 1;
        reverse_int |= g & 1;
        g >>= 1;
    }
    return reverse_int;
}

void read_bucket_to_stash(client_ctx *ctx ,int bucket_id,
                unsigned char *socket_buf) {
    int i;
    oram_bucket_metadata meda;
    oram_bucket_metadata *meta = &meda;
    socket_ctx *sock_ctx = (socket_ctx *)socket_buf;
    socket_read_bucket *sock_read = (socket_read_bucket *)sock_ctx->buf;
    socket_read_bucket_r *sock_read_r = (socket_read_bucket_r *)sock_ctx->buf;
    stash_block *block = malloc(sizeof(stash_block));
    sock_read->bucket_id = bucket_id;
    send(ctx->socket, socket_buf, ORAM_SOCKET_READ_SIZE, 0);
    recv(ctx->socket, socket_buf, ORAM_SOCKET_BUFFER, 0);
    decrypt_message((unsigned char *)meta, sock_read_r->bucket.encrypt_metadata, ORAM_CRYPT_META_SIZE);
    for (i = 0;i < ORAM_BUCKET_REAL;i++) {
        //invalid address is set to -1
        if (sock_read_r->bucket.valid_bits[meta->offset[i]] == 1 && meta->address[meta->offset[i]] >= 0) {
            block->address = meta->address[meta->offset[i]];
            block->bucket_id = sock_read_r->bucket_id;
            decrypt_message(block->data, sock_read_r->bucket.data[meta->offset[i]], ORAM_BLOCK_SIZE);
            add_to_stash(ctx->stash, block);
        }
    }
}

void write_bucket_to_server(client_ctx *ctx, int bucket_id,
                 unsigned char *socket_buf) {
    int count,i;
    oram_bucket_metadata meda;
    oram_bucket_metadata *meta = &meda;
    socket_ctx *sock_ctx = (socket_ctx *)socket_buf;
    socket_write_bucket *sock_write = (socket_write_bucket *)sock_ctx->buf;
    stash_block **stash_list = calloc(ORAM_BUCKET_REAL, sizeof(stash_block));
    count = find_remove_by_bucket(ctx->stash, bucket_id, ORAM_BUCKET_REAL, stash_list);
    get_random_permutation(ORAM_BUCKET_SIZE, meta->offset);
    for (i = 0;i < count;i++) {
        encrypt_message(sock_write->bucket.data[meta->offset[i]], stash_list[i]->data, ORAM_BLOCK_SIZE);
        meta->address[i] = stash_list[i]->address;
    }
    for (i = count; i < ORAM_BUCKET_SIZE; i++) {
        encrypt_message(sock_write->bucket.data[meta->offset[i]], ctx->blank_data, ORAM_BLOCK_SIZE);
        if (i < ORAM_BUCKET_REAL)
            meta->address[i] = -1;
    }
    sock_write->bucket.read_counter = 0;
    memset(sock_write->bucket.valid_bits, 1, sizeof(sock_write->bucket.valid_bits));
    encrypt_message(sock_write->bucket.encrypt_metadata, (unsigned char *)meta, ORAM_META_SIZE);
    send(ctx->socket, socket_buf, SOCKET_WRITE_BUCKET, 0);
}

int get_metadata_helper(int pos, unsigned char *socket_buf, oram_bucket_encrypted_metadata metadata[], client_ctx *ctx) {
    int pos_run, i;
    socket_ctx *sock_ctx = (socket_ctx *)socket_buf;
    socket_get_metadata *sock_meta = (socket_get_metadata *)sock_ctx->buf;
    socket_get_metadata_r *sock_meta_r = (socket_get_metadata_r *)sock_ctx->buf;
    sock_ctx->type = SOCKET_GET_META;
    sock_meta->pos = pos;
    send(ctx->socket, socket_buf, ORAM_SOCKET_META_SIZE, 0);
    recv(ctx->socket, socket_buf, ORAM_SOCKET_BUFFER, 0);
    for (i = 0, pos_run = pos; ; pos_run >>= 1, ++i) {
        if (decrypt_message(metadata[i].encrypt_metadata, sock_meta_r->metadata[i].encrypt_metadata, ORAM_CRYPT_META_SIZE) == -1)
            return -1;
        metadata[i].read_counter = sock_meta_r->metadata[i].read_counter;
        memcpy(metadata[i].valid_bits, sock_meta_r->metadata[i].valid_bits, sizeof(metadata[0].valid_bits));
        if (pos_run == 0)
            break;
    }
    return 0;
}

int read_block_helper(int pos, int address, unsigned char socket_buf[],
                      oram_bucket_encrypted_metadata metadata[],
                      unsigned char data[], client_ctx *ctx) {
    int found, i, j, pos_run, found_pos = -1;
    unsigned char xor_tem[ORAM_CRYPT_DATA_SIZE];
    socket_ctx *sock_ctx = (socket_ctx *)socket_buf;
    socket_read_block *sock_block = (socket_read_block *)sock_ctx->buf;
    socket_read_block_r *sock_block_r = (socket_read_block_r *)sock_ctx->buf;
    sock_ctx->type = SOCKET_READ_BLOCK;
    sock_block->pos = pos;
    oram_bucket_metadata *de_meta;
    for (i = 0, found = 0, pos_run = pos; ; pos_run >>= 1, ++i) {
        de_meta = (oram_bucket_metadata *)&metadata[i].encrypt_metadata;
        if (found == 1)
            sock_block->offsets[i] = get_random_dummy(metadata[i].valid_bits, de_meta->offset);
        else {
            for (j = 0; j < ORAM_BUCKET_REAL; j++) {
                if (de_meta->address[j] == address && metadata[j].valid_bits[j] == 1) {
                    sock_block->offsets[i] = de_meta->offset[j];
                    found = 1;
                    found_pos = i;
                    break;
                }
            }
        }
        if (pos_run == 0)
            break;
    }
    send(ctx->socket, (void *)socket_buf, ORAM_SOCKET_BLOCK_SIZE, 0);
    recv(ctx->socket, (void *)socket_buf, ORAM_SOCKET_BLOCK_SIZE_R, 0);
    //TODO Bug exists when i=0 and pos_run = 0
    for (i = 0, pos_run = pos;;pos_run >>= 1, ++i) {
        if (i == found_pos)
            continue;
        encrypt_message_old(xor_tem, ctx->blank_data, ORAM_BLOCK_SIZE, sock_block_r->nonce[i]);
        for (j = 0;j < ORAM_CRYPT_DATA_SIZE;j++)
            sock_block_r->data[j] ^= xor_tem[j];
        if (pos_run == 0)
            break;
    }
    decrypt_message_old(data, sock_block_r->data, ORAM_CRYPT_DATA_SIZE, sock_block_r->nonce[found_pos]);
    if (found != 1)
        logf("dont");
    return found;
}

int read_path(int pos, int address, unsigned char data[], client_ctx *ctx) {
    unsigned char socket_buf[ORAM_SOCKET_BUFFER];
    oram_bucket_encrypted_metadata metadata[ORAM_TREE_DEPTH];
    if (get_metadata_helper(pos, socket_buf, metadata, ctx) == -1){
        return -1;
    }
    int found = read_block_helper(pos, address, socket_buf, metadata, data, ctx);
    early_reshuffle(pos, metadata, ctx);
    return found;
}

void access(int address, oram_access_op op, unsigned char data[], client_ctx *ctx) {
    int position_new, position, data_in;
    unsigned char read_data[ORAM_BLOCK_SIZE];
    stash_block *block;
    position_new = get_random(ctx->oram_size);
    position = ctx->position_map[address];

    data_in = read_path(position, address, read_data, ctx);
    if (data_in == 0)
        block = find_remove_by_address(ctx->stash, address);
    else if (data_in == 1) {
        block = malloc(sizeof(stash_block));
        block->address = (unsigned int) address;
    }
    else
        return;
    if (op == ORAM_ACCESS_READ)
        memcpy(data, block->data, ORAM_BLOCK_SIZE);
    else
        memcpy(block->data, data, ORAM_BLOCK_SIZE);
    block->bucket_id = position_new;
    add_to_stash(ctx->stash, block);
    //DO NOT Reshuffle in the first time
    ctx->round = (++ctx->round) % ORAM_RESHUFFLE_RATE;
    if (ctx->round == 0)
        evict_path(ctx);
}

void oram_server_init(int bucket_size, client_ctx *ctx) {
    unsigned char buf[4096];
    socket_ctx *sock_ctx = (socket_ctx *)buf;
    socket_init *sock_init_ctx = (socket_init *)sock_ctx->buf;
    sock_ctx->type = SOCKET_INIT;
    sock_init_ctx->size = bucket_size;
    send(ctx->socket, buf, ORAM_SOCKET_INIT_SIZE, 0);
    logf("Init Request to Server");
}

void evict_path(client_ctx *ctx) {
    unsigned char socket_buf[ORAM_SOCKET_BUFFER];
    int pos_run;
    socket_ctx *sock_ctx = (socket_ctx *)socket_buf;
    int pos = gen_reverse_lexicographic(ctx->eviction_g);
    ctx->eviction_g = (ctx->eviction_g++) % ORAM_LEAF_SIZE;
    sock_ctx->type = SOCKET_READ_BUCKET;
    for (pos_run = pos; ;pos_run >>= 1) {
        read_bucket_to_stash(ctx, pos_run, socket_buf);
        if (pos_run == 0)
            break;
    }

    sock_ctx->type = SOCKET_WRITE_BUCKET;
    for (pos_run = pos; ;pos_run >>= 1) {
        write_bucket_to_server(ctx, pos_run, socket_buf);
        if (pos_run == 0)
            break;
    }
}

void early_reshuffle(int pos, oram_bucket_encrypted_metadata metadata[], client_ctx *ctx) {
    int i, pos_run;
    unsigned char socket_buf[ORAM_SOCKET_BUFFER];
    for (i = 0, pos_run = pos;; pos_run >>= 1, ++i) {
        if (metadata[i].read_counter >= ORAM_BUCKET_DUMMY) {
            read_bucket_to_stash(ctx, pos_run, socket_buf);
            write_bucket_to_server(ctx, pos_run, socket_buf);
        }
        if (pos_run == 0)
            break;
    }
}

void client_init(client_ctx *ctx, int size_bucket, oram_args_t *args) {
    int i, bk, j, w;
    int address_size = size_bucket * ORAM_BUCKET_REAL;
    //TODO decide if 40 is enough
    int address_position_map[size_bucket][80];
    unsigned char socket_buf[ORAM_SOCKET_BUFFER];
    socket_ctx *sock_ctx = (socket_ctx *)socket_buf;
    socket_write_bucket *sock_write = (socket_write_bucket *)sock_ctx->buf;
    oram_bucket *bucket = &sock_write->bucket;
    oram_bucket_metadata metadata;
    ctx->oram_size = size_bucket;
    ctx->position_map = malloc(sizeof(int) * address_size);
    ctx->stash = malloc(sizeof(client_stash));
    stash_block *block;
    bzero(ctx->stash, sizeof(client_stash));
    init_stash(ctx->stash);
    sock_init(&ctx->server_addr, &ctx->addrlen, &ctx->socket, "127.0.0.1", args->port, 0);

    //assign block to random bucket
    for (i = 0; i < size_bucket*ORAM_BUCKET_REAL;i++) {
        bk = get_random(size_bucket);
        address_position_map[bk][++address_position_map[bk][0]] = i;
        ctx->position_map[i] = bk;
    }

    oram_server_init(size_bucket, ctx);

    //construct a new bucket
    bucket->read_counter = 0;
    memset(bucket->valid_bits, 1, sizeof(bucket->valid_bits));
    for (i = 0; i < size_bucket; i++) {
        get_random_permutation(ORAM_BUCKET_SIZE, metadata.offset);
        for (j = 0; j < address_position_map[i][0] && j < ORAM_BUCKET_REAL; j++) {
            encrypt_message(bucket->data[metadata.offset[j]], ctx->blank_data, ORAM_BLOCK_SIZE);
            metadata.address[j] = address_position_map[i][j+1];
        }
        //Extra Blocks are written into stash
        for (w = ORAM_BUCKET_REAL;w < address_position_map[i][0];++w) {
            block = malloc(sizeof(stash_block));
            block->bucket_id = i;
            block->address = address_position_map[i][j+1];
            memcpy(block->data, ctx->blank_data, ORAM_BLOCK_SIZE);
            add_to_stash(ctx->stash, block);
        }
        for (; j < ORAM_BUCKET_SIZE; j++) {
            encrypt_message(bucket->data[metadata.offset[j]], ctx->blank_data, ORAM_BLOCK_SIZE);
            if (j < ORAM_BUCKET_REAL)
                metadata.address[j] = -1;
        }

        encrypt_message(bucket->encrypt_metadata, (unsigned char *)&metadata, ORAM_META_SIZE);
        sock_ctx->type = SOCKET_WRITE_BUCKET;
        sock_write->bucket_id = i;
        int r = send(ctx->socket, socket_buf, ORAM_SOCKET_WRITE_SIZE, 0);
    }
    logf("Client Init");
}
