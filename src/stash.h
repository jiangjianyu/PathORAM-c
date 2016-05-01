//
// Created by jyjia on 2016/4/30.
//

#ifndef PATHORAM_STASH_H
#define PATHORAM_STASH_H

#include "utlist.h"
#include "uthash.h"
#include "bucket.h"

typedef struct stash_block{
    unsigned int address;
    unsigned int bucket_id;
    unsigned char data[ORAM_BLOCK_SIZE];
    struct stash_block *next;
    UT_hash_handle hh;
} stash_block;

//two hash table, indexed by bucket_id or address
typedef struct {
    stash_block *bucket_to_stash[ORAM_TOTAL_BUCKET];
    int bucket_to_stash_count[ORAM_TOTAL_BUCKET];
    stash_block *address_to_stash;
} client_stash;

int init_stash(client_stash *stash);

void add_to_stash(client_stash *stash ,stash_block *block);

//return remove block
int find_remove_by_bucket(client_stash *stash, int bucket_id, int max, stash_block *block_list[]);

stash_block* find_remove_by_address(client_stash *stash, int address);

#endif //PATHORAM_STASH_H
