//
// Created by jyjia on 2016/4/29.
//

#ifndef PATHORAM_SERVER_H
#define PATHORAM_SERVER_H

#include "bucket.h"
#include "args.h"

typedef struct {
    int socket;
    struct sockaddr_in client_addr;
    socklen_t addrlen;
    struct sockaddr_in server_addr;
    int running;
    unsigned char *buff;
} server_ctx;

int read_bucket(sock_ctx *sock_ctx, int bucket_id);

int write_bucket(sock_ctx *sock_ctx, int bucket_id);

int get_metadata(int pos, oram_bucket_metadata metadata[]);

int read_block(server_ctx *ctx, int pos, int bucket_offset_list[]);

int server_run(oram_args_t *args);

#endif //PATHORAM_SERVER_H
