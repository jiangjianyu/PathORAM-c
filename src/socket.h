//
// Created by jyjia on 2016/4/29.
//

#ifndef PATHORAM_SOCKET_H
#define PATHORAM_SOCKET_H

#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include "bucket.h"

#define ORAM_SOCKET_TIMEOUT
#define ORAM_SOCKET_TIMEOUT_SECOND 5
#define ORAM_SOCKET_TIMEOUT_USECOND 0

typedef enum {
    SOCKET_READ_BUCKET = 0,
    SOCKET_WRITE_BUCKET = 1,
    SOCKET_READ_PATH = 2,
    SOCKET_WRITE_PATH = 3,
    SOCKET_GET_META = 4,
    SOCKET_READ_BLOCK = 5,
    SOCKET_INIT = 6,
} socket_type;

typedef enum {
    SOCKET_RESPONSE_SUCCESS = 0,
    SOCKET_RESPONSE_FAIL = 1
} socket_response_type;

typedef enum {
    SOCKET_OP_CREATE = 0,
    SOCKET_OP_LOAD = 1,
    SOCKET_OP_SAVE = 2,
    SOCKET_OP_REINIT = 4
} oram_init_op;

typedef struct {
    socket_type type;
    unsigned char buf[];
} socket_ctx;

typedef struct {
    int bucket_id;
    oram_bucket bucket;
} socket_write_bucket;

typedef struct {
    int bucket_id;
    socket_response_type type;
} socket_write_bucket_r;

typedef struct {
    int bucket_id;
} socket_read_bucket;

typedef struct {
    int bucket_id;
    oram_bucket bucket;
} socket_read_bucket_r;

typedef struct {
    int pos;
    int len;
} socket_path_header;

typedef struct {
    int pos;
} socket_get_metadata;

typedef struct {
    int pos;
    oram_bucket_encrypted_metadata metadata[ORAM_TREE_DEPTH];
} socket_get_metadata_r;

typedef struct {
    int pos;
    int offsets[ORAM_TREE_DEPTH];
} socket_read_block;

typedef struct {
    int pos;
    unsigned char data[ORAM_CRYPT_DATA_SIZE];
    unsigned char nonce[ORAM_TREE_DEPTH][ORAM_CRYPT_NONCE_LEN];
} socket_read_block_r;

//server will delete all server data when re_init is 1(true) and server is inited
typedef struct {
    oram_init_op op;
    int size;
    char storage_key[ORAM_STORAGE_KEY_LEN];
} socket_init;

typedef struct {
    socket_response_type status;
    char err_msg[20];
} socket_init_r;

typedef struct {
    int address;
    oram_access_op op;
    unsigned char data[ORAM_BLOCK_SIZE];
} socket_access;

typedef struct {
    int address;
    socket_response_type status;
    unsigned char data[ORAM_BLOCK_SIZE];
} socket_access_r;

#define ORAM_SOCKET_OVERHEAD sizeof(int)
#define ORAM_SOCKET_READ_SIZE sizeof(socket_read_bucket) + ORAM_SOCKET_OVERHEAD
#define ORAM_SOCKET_META_SIZE sizeof(socket_get_metadata) + ORAM_SOCKET_OVERHEAD
#define ORAM_SOCKET_PATH_SIZE sizeof(socket_path_header) + ORAM_SOCKET_OVERHEAD
#define ORAM_SOCKET_BLOCK_SIZE(tree_height) sizeof(socket_read_block) + ORAM_SOCKET_OVERHEAD \
                                                - (ORAM_TREE_DEPTH - tree_height) * sizeof(int)
#define ORAM_SOCKET_WRITE_SIZE sizeof(socket_write_bucket) + ORAM_SOCKET_OVERHEAD
#define ORAM_SOCKET_INIT_SIZE sizeof(socket_init) + ORAM_SOCKET_OVERHEAD

#define ORAM_SOCKET_READ_SIZE_R sizeof(socket_read_bucket_r) + ORAM_SOCKET_OVERHEAD
#define ORAM_SOCKET_META_SIZE_R(tree_height) sizeof(socket_get_metadata_r) + ORAM_SOCKET_OVERHEAD \
                                                 - (ORAM_TREE_DEPTH - tree_height) * sizeof(oram_bucket_encrypted_metadata)
#define ORAM_SOCKET_BLOCK_SIZE_R(tree_height) sizeof(socket_read_block_r) + ORAM_SOCKET_OVERHEAD \
                                                  - (ORAM_TREE_DEPTH - tree_height) * ORAM_CRYPT_NONCE_LEN
#define ORAM_SOCKET_WRITE_SIZE_R sizeof(socket_write_bucket_r) + ORAM_SOCKET_OVERHEAD
#define ORAM_SOCKET_INIT_SIZE_R sizeof(socket_init_r) + ORAM_SOCKET_OVERHEAD


#define ORAM_SOCKET_BUFFER(tree_height) ORAM_SOCKET_PATH_SIZE + sizeof(oram_bucket) * tree_height

#define ORAM_SOCKET_BUFFER_MAX ORAM_SOCKET_READ_SIZE_R

#define ORAM_SOCKET_BACKLOG 40

#define ORAM_SOCKET_ACCESS_SIZE sizeof(socket_access)
#define ORAM_SOCKET_ACCESS_SIZE_R sizeof(socket_access_r)

int sock_init_byhost(struct sockaddr_in *addr, socklen_t *addrlen, int *sock,
                 char *host, int port, int if_bind);

void sock_init(struct sockaddr_in *addr, socklen_t *addrlen, char *host, int port);

int sock_connect_init(struct sockaddr_in *addr, socklen_t addrlen);

int sock_standard_send(int sock, unsigned char send_msg[], int len);

int sock_standard_recv(int sock, unsigned char recv_msg[], int sock_len);
//recv left package
int sock_recv_add(int sock, unsigned char recv_msg[], int now, int total);

int sock_send_recv(int sock, unsigned char send_msg[], unsigned char recv_msg[],
                   int send_len, int recv_len);

#endif //PATHORAM_SOCKET_H
