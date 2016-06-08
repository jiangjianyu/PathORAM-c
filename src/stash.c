//
// Created by jyjia on 2016/4/30.
//

#include "stash.h"
#include "performance.h"

int init_stash(client_stash *stash, int size) {
    stash->address_to_stash = NULL;
    stash->bucket_to_stash = calloc(size, sizeof(stash_block *));
    stash->bucket_to_stash_count = calloc(size, sizeof(int));
    return 1;
}

void add_to_stash(client_stash *stash, stash_block *block) {
    P_ADD_STASH(1);
    stash_block *ne;
    HASH_FIND_INT(stash->address_to_stash, &block->address, ne);
    //Does not add into hash table when exists
    if (ne != NULL) {
        log_f("already %d", block->address);
        return;
    }
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
        next = now->next_l;
        block_list[i] = now;
        HASH_DEL(stash->address_to_stash, now);
        LL_DELETE(stash->bucket_to_stash[bucket_id], now);
        stash->bucket_to_stash_count[bucket_id]--;
    }
    P_DEL_STASH(delete_max);
    return delete_max;
}

stash_block* find_remove_by_address(client_stash *stash, int address) {
    stash_block *return_block;
    HASH_FIND_INT(stash->address_to_stash, &address, return_block);
    HASH_DEL(stash->address_to_stash, return_block);
    LL_DELETE(stash->bucket_to_stash[return_block->bucket_id], return_block);
    stash->bucket_to_stash_count[return_block->bucket_id]--;
    return return_block;
}