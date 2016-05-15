//
// Created by jyjia on 2016/4/30.
//

#ifndef PATHORAM_STASH_H
#define PATHORAM_STASH_H

#include "utlist.h"
#include "uthash.h"
#include "bucket.h"
#include "log.h"

typedef struct stash_block{
    int address;
    int *bucket_id;
    int evict_count;
    _Bool write_after_evict;
    unsigned char data[ORAM_BLOCK_SIZE];
    UT_hash_handle hh;
} stash_block;

typedef struct stash_list_block {
    stash_block *block;
    struct stash_list_block *next_l;
} stash_list_block;

//two hash table, indexed by bucket_id or address
typedef struct {
    stash_list_block **bucket_to_stash;
    int *bucket_to_stash_count;
    stash_block *address_to_stash;
    pthread_mutex_t stash_mutex;
} client_stash;

int init_stash(client_stash *stash, int size);

void add_to_stash(client_stash *stash ,stash_block *block);

//return remove block
int find_remove_by_bucket(client_stash *stash, int bucket_id, int max, stash_block *block_list[]);

stash_block* find_edit_by_address(client_stash *stash, int address, oram_access_op op, unsigned char data[]);

#endif //PATHORAM_STASH_H
