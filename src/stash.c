//
// Created by jyjia on 2016/4/30.
//

#include <pthread.h>
#include "stash.h"
#include "client.h"
#include "performance.h"

int in_bucket_list(int tar, int mem[], int start, int len) {
    int i;
    for (i = start;i < len;i++) {
        if (mem[i] == tar)
            return i;
    }
    return -1;
}

int init_stash(client_stash *stash, int size) {
    stash->address_to_stash = NULL;
    stash->bucket_to_stash = calloc(size, sizeof(stash_block *));
    stash->bucket_to_stash_count = calloc(size, sizeof(int));
    pthread_mutex_init(&stash->stash_mutex, NULL);
    return 1;
}


//Only add to stash if in bucket_id list
void add_to_stash(client_stash *stash, stash_block *block) {
    P_ADD_STASH(1);
    stash_block *ne;
    stash_list_block *ne_list;
    int i, j, k;
    pthread_mutex_lock(&stash->stash_mutex);
    HASH_FIND_INT(stash->address_to_stash, &block->address, ne);
    //Does not add into hash table when exists
    if (ne != NULL) {
        if (ne->evict_count < client_t.backup_count) {
            int index = in_bucket_list(block->bucket_id[0], ne->bucket_id, ne->evict_count,client_t.backup_count);
            if (index >= 0) {
                if (index != ne->evict_count) {
                    int tem = ne->bucket_id[index];
                    ne->bucket_id[index] = ne->bucket_id[ne->evict_count];
                    ne->bucket_id[ne->evict_count++] = tem;
                }
                else
                    ne->evict_count++;
                log_all("Address %d in bucket %d", block->address, block->bucket_id[0], block->data[0]);
                ne_list = malloc(sizeof(stash_list_block));
                ne_list->block = ne;
                ne_list->next_l = NULL;
                LL_APPEND(stash->bucket_to_stash[block->bucket_id[0]], ne_list);
                stash->bucket_to_stash_count[block->bucket_id[0]]++;
            }
        }
        pthread_mutex_unlock(&stash->stash_mutex);
        return;
    }
    int index = in_bucket_list(block->bucket_id[0],
                               client_t.position_map + block->address * client_t.backup_count,
                               0,client_t.backup_count);
    if (index >= 0) {
        HASH_ADD_INT(stash->address_to_stash, address, block);
        for (i = 0; i < block->evict_count; i++) {
            log_all("Address %d in bucket %d", block->address, block->bucket_id[i], block->data[0]);
            ne_list = malloc(sizeof(stash_list_block));
            ne_list->block = block;
            ne_list->next_l = NULL;
            LL_APPEND(stash->bucket_to_stash[block->bucket_id[i]], ne_list);
            stash->bucket_to_stash_count[block->bucket_id[i]]++;
        }
        for (k=0;i < client_t.backup_count && k < client_t.backup_count;k++) {
            int same = 0;
            for (j = 0; j < block->evict_count;j++) {
                if (client_t.position_map[block->address * client_t.backup_count + k] == block->bucket_id[j])
                    same = 1;
            }
            if (!same)
                block->bucket_id[i++] = client_t.position_map[block->address * client_t.backup_count + k];
        }
    }
    else {
        log_all("duplicated stash block");
    }
    pthread_mutex_unlock(&stash->stash_mutex);
}

//return remove block
int find_remove_by_bucket(client_stash *stash, int bucket_id, int max, stash_block *block_list[]) {
    pthread_mutex_lock(&stash->stash_mutex);
    int delete_max = stash->bucket_to_stash_count[bucket_id];
    stash_list_block *now, *next = stash->bucket_to_stash[bucket_id], *new_list;
    stash_block *now_block;
    int i = 0, j;
    if (delete_max > max)
        delete_max = max;
    for (;i < delete_max; i++) {
        now = next;
        next = now->next_l;
        now_block = now->block;
        block_list[i] = now_block;
        log_all("Address %d not in bucket %d", now_block->address, bucket_id, now_block->data[0]);
        if (now->block->evict_count == 1) {
            HASH_DEL(stash->address_to_stash, now_block);
            LL_DELETE(stash->bucket_to_stash[bucket_id], now);
            stash->bucket_to_stash_count[bucket_id]--;
            if (now_block->write_after_evict == 1) {
                HASH_ADD_INT(stash->address_to_stash, address, now_block);
                now_block->evict_count = client_t.backup_count;
                now_block->write_after_evict = 0;
                for (j = 0; j < client_t.backup_count; j++) {
                    log_all("RE:Address %d in bucket %d", now_block->address, now_block->bucket_id[j]);
                    new_list = malloc(sizeof(stash_list_block));
                    new_list->block = now_block;
                    new_list->next_l = NULL;
                    LL_APPEND(stash->bucket_to_stash[now_block->bucket_id[j]], new_list);
                    stash->bucket_to_stash_count[now_block->bucket_id[j]]++;
                }
            }
        }
        else {
            int index = in_bucket_list(bucket_id, now_block->bucket_id, 0,client_t.backup_count);
            if (index >= now_block->evict_count || index < 0) {
                log_detail("stash error:index %d, bucket %d", index, bucket_id);
                return -1;
            }
            //Last valid index
            now_block->evict_count--;
            if (index != now_block->evict_count) {
                int tem = now_block->bucket_id[index];
                now_block->bucket_id[index] = now_block->bucket_id[now_block->evict_count];
                now_block->bucket_id[now_block->evict_count] = tem;
            }
            LL_DELETE(stash->bucket_to_stash[bucket_id], now);
            stash->bucket_to_stash_count[bucket_id]--;
        }
    }
    pthread_mutex_unlock(&stash->stash_mutex);
    P_DEL_STASH(delete_max);
    return delete_max;
}

stash_block* find_edit_by_address(client_stash *stash, int address, oram_access_op op, unsigned char data[]) {
    stash_block *return_block;
    pthread_mutex_lock(&stash->stash_mutex);
    HASH_FIND_INT(stash->address_to_stash, &address, return_block);
    if (return_block != NULL) {
        if (op == ORAM_ACCESS_WRITE) {
            memcpy(return_block->data, data, ORAM_BLOCK_SIZE);
            if (return_block->evict_count < client_t.backup_count)
                return_block->write_after_evict = 1;
        }
        else
            memcpy(data, return_block->data, ORAM_BLOCK_SIZE);
//        HASH_DEL(stash->address_to_stash, return_block);
//        for (i = 0; i < return_block->evict_count; i++) {
//            LL_DELETE(stash->bucket_to_stash[return_block->bucket_id[i]], return_block);
//            stash->bucket_to_stash_count[return_block->bucket_id[i]]--;
//        }
    }
    pthread_mutex_unlock(&stash->stash_mutex);
    return return_block;
}

int stash_block_lock(client_stash *stash , int address, oram_lock_status status) {
    stash_block *ne;
    int return_status = 0;
    pthread_mutex_lock(&stash->stash_mutex);
    HASH_FIND_INT(stash->address_to_stash, &address, ne);
    if (ne != NULL) {
        ne->lock_status = status;
        return_status = 1;
    }
    pthread_mutex_unlock(&stash->stash_mutex);
    return return_status;
}

int stash_block_unlock(client_stash *stash , int address) {
    stash_block *ne;
    int status = 0;
    pthread_mutex_lock(&stash->stash_mutex);
    HASH_FIND_INT(stash->address_to_stash, &address, ne);
    if (ne != NULL) {
        ne->lock_status = ORAM_LOCK_UNLOCK;
        status = 1;
    }
    pthread_mutex_unlock(&stash->stash_mutex);
    return status;
}
