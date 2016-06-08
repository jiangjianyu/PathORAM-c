//
// Created by jyjia on 2016/4/30.
//

#include <pthread.h>
#include "stash.h"
#include "client.h"
#include "performance.h"
#include <math.h>
#define min(a,b) (((a)<(b)) ? (a):(b))

int get_ith_leaf(int node, int order) {
    int pos = node;
    while(pos < client_t.oram_tree_leaf_start) {
        pos = pos * 2 + (order & 0x01) + 1;
        order >>= 1;
    }
    return pos;
}

int get_max_leaf(int node) {
    int i = 0, pos_run;
    if (node >= client_t.oram_tree_leaf_start)
        return 1;
    for (i = 0, pos_run = node;pos_run < client_t.oram_size;pos_run = pos_run * 2 + 1, i++);
    return (int)pow(2, i -1);
}

int address_in_path(int address_pos, int node_pos) {
    int index_a = (address_pos/client_t.oram_size) % client_t.node_count;
    int index_b = (node_pos/client_t.oram_size) % client_t.node_count;
    if (index_a != index_b)
        return 0;
    int index_node = address_pos % client_t.oram_size;
    int index_leaf = node_pos % client_t.oram_size;
    int pos_run = index_leaf;
    for (;;pos_run = (pos_run - 1) >> 1 ) {
        if (pos_run == index_node)
            return 1;
        if (pos_run < index_node)
            break;
    }
    return 0;
}

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

    stash_block *ne;
    stash_list_block *ne_list;

    pthread_mutex_lock(&stash->stash_mutex);
    HASH_FIND_INT(stash->address_to_stash, &block->address, ne);
    //Does not add into hash table when exists

    if (ne == NULL) {
        if (address_in_path(block->bucket_id,
                            client_t.position_map[block->address])) {
            P_ADD_STASH(1);
            block->evict_bool = malloc(client_t.backup_count * sizeof(_Bool));
            bzero(block->evict_bool, sizeof(_Bool) * client_t.backup_count);
            block->bucket_id = client_t.position_map[block->address];
            HASH_ADD_INT(stash->address_to_stash, address, block);
            ne_list = malloc(sizeof(stash_list_block));
            ne_list->block = block;
            ne_list->next_l = NULL;
            LL_APPEND(stash->bucket_to_stash[block->bucket_id], ne_list);
            stash->bucket_to_stash_count[block->bucket_id]++;
//            log_detail("Add block %d to stash, bucket %d", block->address, block->bucket_id);
        } else {
            log_detail("Too old, drop block");
        }
    } else {
        if (address_in_path(block->bucket_id,
                            client_t.position_map[block->address])) {
            ne->evict_bool[(block->bucket_id / client_t.oram_size) / client_t.backup_count] = 0;
        }
    }
    pthread_mutex_unlock(&stash->stash_mutex);
    return;

    /*
    int i, j, k;
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
     */
}

//return remove block
int find_remove_by_bucket(client_stash *stash, int bucket_id, int max, stash_block *block_list[], int backup_index) {
    int i = 0, j = 0,delete_max = 0, leaf, delete_now, left, m, q, q_finish;

    pthread_mutex_lock(&stash->stash_mutex);
    stash_list_block *now, *next;
    stash_block *now_block;
    int bucket_id_node = bucket_id / client_t.oram_size;
    int bucket_id_index = bucket_id % client_t.oram_size;
    int max_leaf = get_max_leaf(bucket_id_index);
    for (m = 0;i < max_leaf && delete_max < max;i++) {
        leaf = get_ith_leaf(bucket_id_index, i) + bucket_id_node * client_t.oram_size;
        delete_now = stash->bucket_to_stash_count[leaf];
        if (delete_now == 0)
            continue;
        left = max - delete_max;
        next = stash->bucket_to_stash[leaf];
        for (;j < min(left, delete_now);j++) {
            delete_max++;
            now = next;
            next = now->next_l;
            now_block = now->block;
            now_block->evict_bool[backup_index] = 1;
            block_list[m++] = now_block;
            log_detail("Evict block %d to bucket %d i %d", now_block->address, now_block->bucket_id, i);
            q_finish = 1;
            for (q = 0;q < client_t.backup_count;q++) {
                if (!now_block->evict_bool[q]) {
                    q_finish = 0;
                    break;
                }
            }
            if(q_finish) {
                log_detail("Evict block %d to server, all finished", now_block->address);
                HASH_DEL(stash->address_to_stash, now_block);
                LL_DELETE(stash->bucket_to_stash[leaf], now);
                stash->bucket_to_stash_count[leaf]--;
                P_DEL_STASH(1);
            }
        }
    }
    pthread_mutex_unlock(&stash->stash_mutex);
    log_all("remove %d buckets to server, backup %d", delete_max, backup_index);
    return delete_max;
/*
 *
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
    */
}

stash_block* find_edit_by_address(client_stash *stash, int address, oram_access_op op, unsigned char data[]) {
    stash_block *return_block;
    pthread_mutex_lock(&stash->stash_mutex);
    HASH_FIND_INT(stash->address_to_stash, &address, return_block);
    if (return_block != NULL) {
        if (op == ORAM_ACCESS_WRITE) {
            memcpy(return_block->data, data, ORAM_BLOCK_SIZE);
            bzero(return_block->evict_bool, sizeof(_Bool) * client_t.backup_count);
        }
        else
            memcpy(data, return_block->data, ORAM_BLOCK_SIZE);
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
