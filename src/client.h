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

typedef struct {
    int oram_size;
    int *position_map;
    client_stash *stash;
    int round;
    int socket;
    int eviction_g;
    socklen_t addrlen;
    struct sockaddr_in server_addr;
    unsigned char blank_data[ORAM_BLOCK_SIZE];
} client_ctx;

int get_random_dummy(_Bool valid_bits[], unsigned int offsets[]);

int gen_reverse_lexicographic(int g);

void read_bucket_to_stash(client_ctx *ctx ,int bucket_id,
                unsigned char *socket_buf, oram_bucket_metadata *meta);

void write_bucket_to_server(client_ctx *ctx, int bucket_id,
                 unsigned char *socket_buf, oram_bucket_metadata *meta);

void get_metadata_helper(int pos, unsigned char *socket_buf, oram_bucket_encrypted_metadata metadata[], client_ctx *ctx);

int read_block_helper(int pos, int address,unsigned char socket_buf[], oram_bucket_encrypted_metadata metadata[],
                      unsigned char data[], client_ctx *ctx);

int read_path(int pos, int address, unsigned char data[], client_ctx *ctx);

void access(int address, oram_access_op op, unsigned char data[], client_ctx *ctx);

void evict_path(client_ctx *ctx);

void early_reshuffle(int pos, oram_bucket_encrypted_metadata metadata[], client_ctx *ctx);

void client_init(client_ctx *ctx, int size_bucket, oram_args_t *args);

void oram_server_init(int bucket_size, client_ctx *ctx);

#endif //PATHORAM_CLIENT_H
