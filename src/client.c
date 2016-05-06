//
// Created by jyjia on 2016/4/30.
//
#include <math.h>
#include "client.h"

int get_random_leaf(int pos_node, int oram_size) {
    int random = get_random(oram_size);
    while (2*pos_node < oram_size) {
        pos_node = 2 * pos_node + (random & 0x01) + 1;
        random >>= 1;
    }
    return pos_node;
}

int get_random_dummy(_Bool valid_bits[], int offsets[]) {
    int i;
    for (i = ORAM_BUCKET_REAL;i < ORAM_BUCKET_SIZE;i++) {
        if (valid_bits[offsets[i]] == 1)
            return offsets[i];
    }
    logf("error, not enough");
    return 0;
}

int gen_reverse_lexicographic(int g, int oram_size, int tree_height) {
    int i, reverse_int = 0;
    g += oram_size/2 + 1;
    for (i = 0;i < tree_height;++i) {
        reverse_int <<= 1;
        reverse_int |= g & 1;
        g >>= 1;
    }
    if (reverse_int > oram_size)
        reverse_int >>= 1;
    return reverse_int;
}

void read_bucket_to_stash(client_ctx *ctx ,int bucket_id,
                unsigned char *socket_buf) {
    logf("read bucket %d to stash", bucket_id);
    int i;
    oram_bucket_metadata meda;
    oram_bucket_metadata *meta = &meda;
    socket_ctx *sock_ctx = (socket_ctx *)socket_buf;
    socket_read_bucket *sock_read = (socket_read_bucket *)sock_ctx->buf;
    socket_read_bucket_r *sock_read_r = (socket_read_bucket_r *)sock_ctx->buf;
    stash_block *block;
    sock_read->bucket_id = bucket_id;
    sock_ctx->type = SOCKET_READ_BUCKET;
    sock_send_recv(ctx->socket, socket_buf, socket_buf, ORAM_SOCKET_READ_SIZE, ORAM_SOCKET_READ_SIZE_R);
    decrypt_message((unsigned char *)meta, sock_read_r->bucket.encrypt_metadata, ORAM_CRYPT_META_SIZE_DE);
    for (i = 0;i < ORAM_BUCKET_REAL;i++) {
        //invalid address is set to -1
        if (sock_read_r->bucket.valid_bits[meta->offset[i]] == 1 && meta->address[i] >= 0) {
            block = malloc(sizeof(stash_block));
            block->address = meta->address[i];
            block->bucket_id = sock_read_r->bucket_id;
            decrypt_message(block->data, sock_read_r->bucket.data[meta->offset[i]], ORAM_CRYPT_DATA_SIZE_DE);
//            logf("address %d data:%d", block->address, block->data[0]);
            add_to_stash(ctx->stash, block);
//            logf("address %d in bucket %d, stash", block->address, block->bucket_id);
        }
    }
}

void write_bucket_to_server(client_ctx *ctx, int bucket_id,
                 unsigned char *socket_buf) {
    logf("write bucket %d to server", bucket_id);
    int count,i;
    oram_bucket_metadata meda;
    oram_bucket_metadata *meta = &meda;
    socket_ctx *sock_ctx = (socket_ctx *)socket_buf;
    socket_write_bucket *sock_write = (socket_write_bucket *)sock_ctx->buf;
    stash_block **stash_list = calloc(ORAM_BUCKET_REAL, sizeof(stash_block));
    count = find_remove_by_bucket(ctx->stash, bucket_id, ORAM_BUCKET_REAL, stash_list);
    get_random_permutation(ORAM_BUCKET_SIZE, meta->offset);
    for (i = 0;i < count;i++) {
//        logf("address %d data:%d", stash_list[i]->address, stash_list[i]->data[0]);
        encrypt_message(sock_write->bucket.data[meta->offset[i]], stash_list[i]->data, ORAM_BLOCK_SIZE);
        meta->address[i] = stash_list[i]->address;
//        logf("address %d in bucket %d, server", stash_list[i]->address, bucket_id);
    }
    for (i = count; i < ORAM_BUCKET_SIZE; i++) {
        encrypt_message(sock_write->bucket.data[meta->offset[i]], ctx->blank_data, ORAM_BLOCK_SIZE);
        if (i < ORAM_BUCKET_REAL)
            meta->address[i] = -1;
    }
    sock_ctx->type = SOCKET_WRITE_BUCKET;
    sock_write->bucket_id = bucket_id;
    sock_write->bucket.read_counter = 0;
    memset(sock_write->bucket.valid_bits, 1, sizeof(sock_write->bucket.valid_bits));
    encrypt_message(sock_write->bucket.encrypt_metadata, (unsigned char *)meta, ORAM_META_SIZE);
    sock_send_recv(ctx->socket, socket_buf, socket_buf, ORAM_SOCKET_WRITE_SIZE, ORAM_SOCKET_WRITE_SIZE_R);
}
//TODO does not use oram_bucket_encrypted_meta
int get_metadata_helper(int pos, unsigned char *socket_buf, oram_bucket_encrypted_metadata metadata[], client_ctx *ctx) {
    int pos_run, i;
    socket_ctx *sock_ctx = (socket_ctx *)socket_buf;
    socket_get_metadata *sock_meta = (socket_get_metadata *)sock_ctx->buf;
    socket_get_metadata_r *sock_meta_r = (socket_get_metadata_r *)sock_ctx->buf;
    sock_ctx->type = SOCKET_GET_META;
    sock_meta->pos = pos;
    sock_send_recv(ctx->socket, socket_buf, socket_buf, ORAM_SOCKET_META_SIZE, ORAM_SOCKET_META_SIZE_R(ctx->oram_tree_height));
    for (i = 0, pos_run = pos; ; pos_run >>= 1, ++i) {
        if (decrypt_message(metadata[i].encrypt_metadata,
                            sock_meta_r->metadata[i].encrypt_metadata,
                            ORAM_CRYPT_META_SIZE_DE) != 0) {
            return -1;
        }
        metadata[i].read_counter = sock_meta_r->metadata[i].read_counter;
        ctx->metadata_counter[i] = metadata[i].read_counter;
        memcpy(metadata[i].valid_bits, sock_meta_r->metadata[i].valid_bits, sizeof(metadata[0].valid_bits));
        if (pos_run == 0)
            break;
    }
    return 0;
}

int read_block_helper(int pos, int address, unsigned char socket_buf[],
                      oram_bucket_encrypted_metadata metadata[],
                      unsigned char data[], client_ctx *ctx) {
    int found = 0, i, j, pos_run, found_pos = -1;
    unsigned char xor_tem[ORAM_CRYPT_DATA_SIZE];
    socket_ctx *sock_ctx = (socket_ctx *)socket_buf;
    socket_read_block *sock_block = (socket_read_block *)sock_ctx->buf;
    socket_read_block_r *sock_block_r = (socket_read_block_r *)sock_ctx->buf;
    sock_ctx->type = SOCKET_READ_BLOCK;
    sock_block->pos = pos;
    oram_bucket_metadata *de_meta;
    for (i = 0, found = 0, pos_run = pos; ; pos_run >>= 1, ++i) {
        de_meta = (oram_bucket_metadata *)&metadata[i].encrypt_metadata;
        //TODO Better Loop
        if (found == 1)
            sock_block->offsets[i] = get_random_dummy(metadata[i].valid_bits, de_meta->offset);
        else {
            for (j = 0; j < ORAM_BUCKET_REAL; j++) {
                if (de_meta->address[j] == address && metadata[i].valid_bits[de_meta->offset[j]] == 1) {
                    sock_block->offsets[i] = de_meta->offset[j];
                    found = 1;
                    found_pos = i;
                    break;
                }
            }
            if (found == 0)
                sock_block->offsets[i] = get_random_dummy(metadata[i].valid_bits, de_meta->offset);
        }
//        logf("Read offset %d in bucket %d", sock_block->offsets[i], pos_run);
        if (pos_run == 0)
            break;
    }
    sock_send_recv(ctx->socket, socket_buf, socket_buf,
                   ORAM_SOCKET_BLOCK_SIZE(ctx->oram_tree_height),
                   ORAM_SOCKET_BLOCK_SIZE_R(ctx->oram_tree_height));
    for (i = 0, pos_run = pos;;pos_run >>= 1, ++i) {
        encrypt_message_old(xor_tem, ctx->blank_data, ORAM_BLOCK_SIZE, sock_block_r->nonce[i]);
        if (i != found_pos)
            for (j = 0; j < ORAM_CRYPT_DATA_SIZE; j++)
                sock_block_r->data[j] ^= xor_tem[j];
        if (pos_run == 0)
            break;
    }
    if (found == 1) {
        memcpy(sock_block_r->data, sock_block_r->nonce[found_pos], ORAM_CRYPT_NONCE_LEN);
        decrypt_message(data, sock_block_r->data, ORAM_CRYPT_DATA_SIZE_DE);
    }
    return found;
}

int read_path(int pos, int address, unsigned char data[], client_ctx *ctx) {
    unsigned char socket_buf[ORAM_SOCKET_BUFFER];
    oram_bucket_encrypted_metadata metadata[ORAM_TREE_DEPTH];
    if (get_metadata_helper(pos, socket_buf, metadata, ctx) == -1){
        return -1;
    }
    int found = read_block_helper(pos, address, socket_buf, metadata, data, ctx);
    return found;
}

void access(int address, oram_access_op op, unsigned char data[], client_ctx *ctx) {
    int position_new, position, data_in, leaf_pos;
    unsigned char read_data[ORAM_BLOCK_SIZE];
    stash_block *block;
    position_new = get_random(ctx->oram_size);
    position = ctx->position_map[address];
    leaf_pos = get_random_leaf(position, ctx->oram_size);
    data_in = read_path(leaf_pos, address, read_data, ctx);
    if (data_in == 0) {
        block = find_remove_by_address(ctx->stash, address);
        logf("Access %d from Stash", address);
    }
    else if (data_in == 1) {
        block = malloc(sizeof(stash_block));
        block->address = address;
        memcpy(block->data, read_data, ORAM_BLOCK_SIZE);
        logf("Access %d from Server", address);
    }
    else {
        logf("Access %d from error", address);
        return;
    }
    if (op == ORAM_ACCESS_READ)
        memcpy(data, block->data, ORAM_BLOCK_SIZE);
    else
        memcpy(block->data, data, ORAM_BLOCK_SIZE);
    block->bucket_id = position_new;
    ctx->position_map[address] = position_new;
    add_to_stash(ctx->stash, block);
    //DO NOT Reshuffle in the first time
    ctx->round = (++ctx->round) % ORAM_RESHUFFLE_RATE;
    if (ctx->round == 0)
        evict_path(ctx);
    early_reshuffle(leaf_pos, ctx);
}

void oram_server_init(int bucket_size, client_ctx *ctx) {
    unsigned char buf[4096];
    socket_ctx *sock_ctx = (socket_ctx *)buf;
    socket_init *sock_init_ctx = (socket_init *)sock_ctx->buf;
    sock_ctx->type = SOCKET_INIT;
    sock_init_ctx->size = bucket_size;
    sock_send_recv(ctx->socket, buf, buf, ORAM_SOCKET_INIT_SIZE, ORAM_SOCKET_INIT_SIZE_R);
    logf("Init Request to Server");
}

void evict_path(client_ctx *ctx) {
    logf("evict path");
    unsigned char socket_buf[ORAM_SOCKET_BUFFER];
    int pos_run;
    socket_ctx *sock_ctx = (socket_ctx *)socket_buf;
    int pos = gen_reverse_lexicographic(ctx->eviction_g, ctx->oram_size, ctx->oram_tree_height);
    ctx->eviction_g = (ctx->eviction_g + 1) % ORAM_LEAF_SIZE;
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

void early_reshuffle(int pos, client_ctx *ctx) {
    int i, pos_run;
    unsigned char socket_buf[ORAM_SOCKET_BUFFER];
    for (i = 0, pos_run = pos;; pos_run >>= 1, ++i) {
        //Notice that read counter in client is always one less
        if (ctx->metadata_counter[i] >= ORAM_BUCKET_DUMMY - 1) {
            logf("early reshuffle %d on pos %d", pos_run, pos_run);
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
    ctx->oram_tree_height = log(ctx->oram_size + 1)/log(2);
    ctx->position_map = malloc(sizeof(int) * address_size);
    ctx->stash = malloc(sizeof(client_stash));
    ctx->round = 0;
    ctx->eviction_g = 0;
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
//            logf("Address %d in bucket %d, server", address_position_map[i][j+1], i);
        }
        //Extra Blocks are written into stash
        for (w = ORAM_BUCKET_REAL;w < address_position_map[i][0];++w) {
            block = malloc(sizeof(stash_block));
            block->bucket_id = i;
            block->address = address_position_map[i][w+1];
            memcpy(block->data, ctx->blank_data, ORAM_BLOCK_SIZE);
            add_to_stash(ctx->stash, block);
//            logf("Address %d in bucket %d, stash", address_position_map[i][j+1], i);
        }
        for (; j < ORAM_BUCKET_SIZE; j++) {
            encrypt_message(bucket->data[metadata.offset[j]], ctx->blank_data, ORAM_BLOCK_SIZE);
            if (j < ORAM_BUCKET_REAL) {
                metadata.address[j] = -1;
            }
        }

        encrypt_message(bucket->encrypt_metadata, (unsigned char *)&metadata, ORAM_META_SIZE);
        sock_ctx->type = SOCKET_WRITE_BUCKET;
        sock_write->bucket_id = i;
        sock_send_recv(ctx->socket, socket_buf, socket_buf, ORAM_SOCKET_WRITE_SIZE, ORAM_SOCKET_WRITE_SIZE_R);
    }
    logf("Client Init");
}
