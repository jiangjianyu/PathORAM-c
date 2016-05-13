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

typedef struct oram_request_queue_block{
    int sock;
    int address;
    oram_access_op op;
    unsigned char data[ORAM_BLOCK_SIZE];
    UT_hash_handle hh;
    struct oram_request_queue_block *next_l;
} oram_request_queue_block;

typedef struct {
    oram_request_queue_block *queue_list;
    oram_request_queue_block *queue_hash;
    pthread_mutex_t queue_mutex;
    sem_t queue_semaphore;
} oram_request_queue;

typedef struct {
    int round;
    int eviction_g;
    struct sockaddr_in server_addr;
    socklen_t addrlen;
    char storage_key[ORAM_STORAGE_KEY_LEN + 1];
} client_storage_ctx;

typedef struct {
    client_storage_ctx **client_storage;
    int oram_size;
    int oram_tree_height;
    int node_count;
    int *server_working_queue;
    int backup_count;
    int **position_map;
    int *node_access_counter;
    pthread_mutex_t mutex_counter;
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

typedef struct access_ctx {
    int sock;
    oram_request_queue_block *queue_block;
    client_storage_ctx *storage_ctx;
    int metadata_counter[ORAM_TREE_DEPTH];
    unsigned char buf[ORAM_SOCKET_BUFFER];
} access_ctx;

client_ctx client_t;

int get_random_leaf(int pos_node, int oram_size);

int get_random_dummy(_Bool valid_bits[], int offsets[]);

int gen_reverse_lexicographic(int g, int tree_size, int tree_height);

void read_bucket_to_stash(access_ctx *ctx ,int bucket_id,
                unsigned char *socket_buf);

void write_bucket_to_server(access_ctx *ctx, int bucket_id,
                 unsigned char *socket_buf);

int get_metadata_helper(int pos, unsigned char *socket_buf, oram_bucket_encrypted_metadata metadata[], access_ctx *ctx);

int read_block_helper(int pos, int address,unsigned char socket_buf[], oram_bucket_encrypted_metadata metadata[],
                      unsigned char data[], access_ctx *ctx);

int read_path(int pos, int address, unsigned char data[], access_ctx *ctx);

int oblivious_access(int address, oram_access_op op, unsigned char data[], access_ctx *ctx);

void evict_path(access_ctx *ctx);

void early_reshuffle(int pos, access_ctx *ctx);

//int client_init(client_ctx *ctx, oram_client_args *args);

int client_create(int node_count,int size_bucket, int backup, int re_init, oram_client_args *args);

int client_load(int re_init);

/*Format of serilized client data

 |  client_ctx  |   client_storage  |   position_map    |   stash_count   | stash   |

 */
void client_save(int exit);

int oram_server_init(int bucket_size, client_storage_ctx *ctx, oram_init_op op);

int listen_accept(oram_client_args *args);

int add_to_queue(int sock, oram_request_queue *queue);

int init_worker();

//Worker function
void * worker_func(void *args);

int client_access(int address, oram_access_op op,unsigned char data[], oram_node_pair *pair);


#endif //PATHORAM_CLIENT_H
