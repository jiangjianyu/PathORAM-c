//
// Created by jyjia on 2016/4/30.
//

#include "stash.h"

int init_stash(client_stash *stash) {
    stash->address_to_stash = NULL;
    bzero(stash->bucket_to_stash, sizeof(stash->bucket_to_stash));
    bzero(stash->bucket_to_stash_count, sizeof(stash->bucket_to_stash_count));
}

void add_to_stash(client_stash *stash, stash_block *block) {
    HASH_ADD_INT(stash->address_to_stash, address, block);
    LL_APPEND(stash->bucket_to_stash[block->bucket_id], block);
    stash->bucket_to_stash_count[block->bucket_id]++;
}

//return remove block
int find_remove_by_bucket(client_stash *stash, int bucket_id, int max, stash_block *block_list[]) {
    int delete_max = stash->bucket_to_stash_count[bucket_id];
    stash_block *now, *next = stash->bucket_to_stash[bucket_id];
    int i = 0;
    if (delete_max > max)
        delete_max = max;
    for (;i < delete_max; i++) {
        now = next;
        next = now->next;
        block_list[i] = now;
        HASH_DEL(stash->address_to_stash, now);
        LL_DELETE(stash->bucket_to_stash[bucket_id], now);
    }
    return delete_max;
}

stash_block* find_remove_by_address(client_stash *stash, int address) {
    stash_block *return_block;
    HASH_FIND_INT(stash->address_to_stash, &address, return_block);
    HASH_DEL(stash->address_to_stash, return_block);
    LL_DELETE(stash->bucket_to_stash[return_block->bucket_id], return_block);
    return return_block;
}