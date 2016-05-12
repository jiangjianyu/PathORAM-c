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
#include "scheduler.h"

#define ORAM_RESHUFFLE_RATE 20
#define ORAM_FILE_CLIENT_FORMAT "ORAM_CLIENT.meta"

typedef struct {
    int socket;
    struct sockaddr_in server_addr;
    socklen_t addrlen;
    char storage_key[ORAM_STORAGE_KEY_LEN + 1];
} client_storage_ctx;

typedef struct {
    client_storage_ctx *client_storage;
    int oram_size;
    int oram_tree_height;
    int node_count;
    int *server_working_queue;
    int backup_count;
    int position_map;
    int round;
    int eviction_g;
    client_stash *stash;
    int server_sock;
    int running;
    struct sockaddr_in server_addr;
    socklen_t addrlen;
    oram_request_queue *queue;
    oram_client_args *args;
    pthread_t *worker_id;
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

int oblivious_access(int address, oram_access_op op, unsigned char data[], client_ctx *ctx);

void evict_path(client_ctx *ctx);

void early_reshuffle(int pos, client_ctx *ctx);

int client_init(client_ctx *ctx, oram_client_args *args);

int client_create(client_ctx *ctx, int size_bucket, int re_init);

int client_load(client_ctx *ctx, int re_init);

void client_save(client_ctx *ctx, int exit);

int oram_server_init(int bucket_size, client_ctx *ctx, oram_init_op op);

#endif //PATHORAM_CLIENT_H
