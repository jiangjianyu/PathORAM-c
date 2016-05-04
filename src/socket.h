//
// Created by jyjia on 2016/4/29.
//

#ifndef PATHORAM_SOCKET_H
#define PATHORAM_SOCKET_H

#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include "bucket.h"



typedef enum {
    SOCKET_READ_BUCKET = 0,
    SOCKET_WRITE_BUCKET = 1,
    SOCKET_GET_META = 2,
    SOCKET_READ_BLOCK = 3,
    SOCKET_INIT = 4,
} socket_type;

typedef enum {
    SOCKET_RESPONSE_SUCCESS = 0,
    SOCKET_RESPONSE_FAIL = 1
} socket_response_type;

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

typedef struct {
    int size;
} socket_init;

typedef struct {
    socket_response_type status;
} socket_init_r;

#define ORAM_SOCKET_OVERHEAD sizeof(int)
#define ORAM_SOCKET_READ_SIZE sizeof(socket_read_bucket) + ORAM_SOCKET_OVERHEAD
#define ORAM_SOCKET_META_SIZE sizeof(socket_get_metadata) + ORAM_SOCKET_OVERHEAD
#define ORAM_SOCKET_BLOCK_SIZE sizeof(socket_read_block) + ORAM_SOCKET_OVERHEAD
#define ORAM_SOCKET_WRITE_SIZE sizeof(socket_write_bucket) + ORAM_SOCKET_OVERHEAD
#define ORAM_SOCKET_INIT_SIZE sizeof(socket_init) + ORAM_SOCKET_OVERHEAD

#define ORAM_SOCKET_READ_SIZE_R sizeof(socket_read_bucket_r) + ORAM_SOCKET_OVERHEAD
#define ORAM_SOCKET_META_SIZE_R sizeof(socket_get_metadata_r) + ORAM_SOCKET_OVERHEAD
#define ORAM_SOCKET_BLOCK_SIZE_R sizeof(socket_read_block_r) + ORAM_SOCKET_OVERHEAD
#define ORAM_SOCKET_WRITE_SIZE_R sizeof(socket_write_bucket_r) + ORAM_SOCKET_OVERHEAD
#define ORAM_SOCKET_INIT_SIZE_R sizeof(socket_init_r) + ORAM_SOCKET_OVERHEAD


#define ORAM_SOCKET_BUFFER ORAM_SOCKET_READ_SIZE_R

#define ORAM_SOCKET_BACKLOG 40

void sock_init(struct sockaddr_in *addr, socklen_t *addrlen, int *sock,
                 char *host, int port, int if_bind);

int sock_standard_send(int sock, unsigned char send_msg[], int len);

int sock_standard_recv(int sock, unsigned char recv_msg[], int sock_len);
//recv left package
int sock_recv_add(int sock, unsigned char recv_msg[], int now, int total);

int sock_send_recv(int sock, unsigned char send_msg[], unsigned char recv_msg[],
                   int send_len, int recv_len);

#endif //PATHORAM_SOCKET_H
