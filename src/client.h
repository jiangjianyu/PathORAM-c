//
// Created by jyjia on 2016/4/29.
//

#ifndef PATHORAM_CLIENT_H
#define PATHORAM_CLIENT_H

#include "bucket.h"
#include "uthash.h"
#include "oram.h"
#include "stash.h"
#include "crypt.h"
#include "socket.h"
#include "args.h"

#define ORAM_RESHUFFLE_RATE 20
#define ORAM_FILE_CLIENT_FORMAT "ORAM_CLIENT.meta"

typedef struct {
    int oram_size;
    int oram_tree_height;
    int *position_map;
    client_stash *stash;
    int round;
    int eviction_g;
} client_storage_ctx;

typedef struct {
    int socket;
    client_storage_ctx *client_storage;
    socklen_t addrlen;
    struct sockaddr_in server_addr;
    //Share buffer for early eviction
    int metadata_counter[ORAM_TREE_DEPTH];
    unsigned char blank_data[ORAM_BLOCK_SIZE];
} client_ctx;


int get_random_leaf(int pos_node, int oram_size);

int get_random_dummy(_Bool valid_bits[], int offsets[]);

int gen_reverse_lexicographic(int g, int tree_size, int tree_height);

void read_bucket_to_stash(client_ctx *ctx ,int bucket_id,
                unsigned char *socket_buf);

void write_bucket_to_server(client_ctx *ctx, int bucket_id,
                 unsigned char *socket_buf);

int get_metadata_helper(int pos, unsigned char *socket_buf, oram_bucket_encrypted_metadata metadata[], client_ctx *ctx);

int read_block_helper(int pos, int address,unsigned char socket_buf[], oram_bucket_encrypted_metadata metadata[],
                      unsigned char data[], client_ctx *ctx);

int read_path(int pos, int address, unsigned char data[], client_ctx *ctx);

void oblivious_access(int address, oram_access_op op, unsigned char data[], client_ctx *ctx);

void evict_path(client_ctx *ctx);

void early_reshuffle(int pos, client_ctx *ctx);

int client_init(client_ctx *ctx, oram_args_t *args);

int client_create(client_ctx *ctx, int size_bucket, int re_init);

int client_load(client_ctx *ctx);

void client_save(client_ctx *ctx);

int oram_server_init(int bucket_size, client_ctx *ctx, oram_init_op op);

#endif //PATHORAM_CLIENT_H
