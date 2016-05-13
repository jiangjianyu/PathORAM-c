//
// Created by jyjia on 2016/4/30.
//

#include "stash.h"
#include "client.h"

int init_stash(client_stash *stash, int size) {
    stash->address_to_stash = NULL;
    stash->bucket_to_stash = calloc(size, sizeof(stash_block *));
    stash->bucket_to_stash_count = calloc(size, sizeof(int));
    return 1;
}


//Still some issue
void add_to_stash(client_stash *stash, stash_block *block) {
    stash_block *ne;
    int i;
    HASH_FIND_INT(stash->address_to_stash, &block->address, ne);
    //Does not add into hash table when exists
    if (ne != NULL) {
        log_f("already %d", block->address);
        if (ne->bucket_count < client_t.backup_count) {
            ne->bucket_id[ne->bucket_count++] = block->bucket_id[0];
            LL_APPEND(stash->bucket_to_stash[block->bucket_id[0]], ne);
        }
        return;
    }
    HASH_ADD_INT(stash->address_to_stash, address, block);
    for (i = 0;i < client_t.backup_count;i++) {
        LL_APPEND(stash->bucket_to_stash[block->bucket_id[i]], block);
        stash->bucket_to_stash_count[block->bucket_id[i]]++;
    }
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
        if (now->evict_count == 1)
            HASH_DEL(stash->address_to_stash, now);
        else
            now->evict_count--;
        LL_DELETE(stash->bucket_to_stash[bucket_id], now);
        stash->bucket_to_stash_count[bucket_id]--;
    }
    return delete_max;
}

stash_block* find_remove_by_address(client_stash *stash, int address) {
    stash_block *return_block;
    int i;
    HASH_FIND_INT(stash->address_to_stash, &address, return_block);
    HASH_DEL(stash->address_to_stash, return_block);
    for (i = 0;i < client_t.backup_count;i++) {
        LL_DELETE(stash->bucket_to_stash[return_block->bucket_id[i]], return_block);
        stash->bucket_to_stash_count[return_block->bucket_id[i]]--;
    }
    return return_block;
}