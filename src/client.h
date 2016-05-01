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

void access(int address, oram_access_op op, unsigned char data[]);

void evict_path();

void early_reshuffle(int bucket_id);

unsigned char* read_path(int pos, int address);

int client_init();

#endif //PATHORAM_CLIENT_H
