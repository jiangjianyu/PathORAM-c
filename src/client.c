//
// Created by jyjia on 2016/4/30.
//
#include <math.h>
#include <fcntl.h>
#include <unistd.h>
#include <pthread.h>
#include "client.h"



int get_random_leaf(int pos_node, int oram_size) {
    int random = get_random(oram_size);
    while (2 * pos_node + 2 < oram_size) {
        pos_node = 2 * pos_node + (random & 0x01) + 1;
        random >>= 1;
    }
    if (2 * pos_node + 1 < oram_size)
        pos_node = 2 * pos_node + 1;
    return pos_node;
}

int get_random_dummy(_Bool valid_bits[], int offsets[]) {
    int i;
    for (i = ORAM_BUCKET_REAL;i < ORAM_BUCKET_SIZE;i++) {
        if (valid_bits[offsets[i]] == 1)
            return offsets[i];
    }
    log_f("error, not enough");
    return 0;
}

int gen_reverse_lexicographic(int g, int oram_size, int tree_height) {
    int i, reverse_int = 0;
    g += oram_size/2 + 1;
    for (i = 0;i < tree_height;++i) {
        reverse_int <<= 1;
        reverse_int |= g & 1;
        g >>= 1;
    }
    if (reverse_int > oram_size)
        reverse_int >>= 1;
    return reverse_int;
}

void read_bucket_to_stash(access_ctx *ctx ,int bucket_id,
                unsigned char *socket_buf) {
    log_f("read bucket %d to stash", bucket_id);
    int i;
    oram_bucket_metadata meda;
    oram_bucket_metadata *meta = &meda;
    socket_ctx *sock_ctx = (socket_ctx *)socket_buf;
    socket_read_bucket *sock_read = (socket_read_bucket *)sock_ctx->buf;
    socket_read_bucket_r *sock_read_r = (socket_read_bucket_r *)sock_ctx->buf;
    stash_block *block;
    sock_read->bucket_id = bucket_id;
    sock_ctx->type = SOCKET_READ_BUCKET;
    sock_send_recv(ctx->sock, socket_buf, socket_buf, ORAM_SOCKET_READ_SIZE, ORAM_SOCKET_READ_SIZE_R);
    decrypt_message((unsigned char *)meta, sock_read_r->bucket.encrypt_metadata, ORAM_CRYPT_META_SIZE_DE);
    for (i = 0;i < ORAM_BUCKET_REAL;i++) {
        //invalid address is set to -1
        if (sock_read_r->bucket.valid_bits[meta->offset[i]] == 1 && meta->address[i] >= 0) {
            block = malloc(sizeof(stash_block));
            block->address = meta->address[i];
            block->bucket_id = malloc(sizeof(int) * client_t.backup_count);
            block->bucket_id[0] = sock_read_r->bucket_id + ctx->access_node * client_t.oram_size;
            block->evict_count = 1;
            decrypt_message(block->data, sock_read_r->bucket.data[meta->offset[i]], ORAM_CRYPT_DATA_SIZE_DE);
//            logf("address %d data:%d", block->address, block->data[0]);
            add_to_stash(client_t.stash, block);
//            log_f("address %d in bucket %d, stash", block->address, block->bucket_id[0]);
        }
    }
}

void write_bucket_to_server(access_ctx *ctx, int bucket_id,
                 unsigned char *socket_buf) {
    log_f("write bucket %d to server", bucket_id);
    int count,i;
    oram_bucket_metadata meda;
    oram_bucket_metadata *meta = &meda;
    socket_ctx *sock_ctx = (socket_ctx *)socket_buf;
    socket_write_bucket *sock_write = (socket_write_bucket *)sock_ctx->buf;
    stash_block **stash_list = calloc(ORAM_BUCKET_REAL, sizeof(stash_block));
    count = find_remove_by_bucket(client_t.stash, bucket_id + ctx->access_node * client_t.oram_size, ORAM_BUCKET_REAL, stash_list);
    get_random_permutation(ORAM_BUCKET_SIZE, meta->offset);
    for (i = 0;i < count;i++) {
//        logf("address %d data:%d", stash_list[i]->address, stash_list[i]->data[0]);
        encrypt_message(sock_write->bucket.data[meta->offset[i]], stash_list[i]->data, ORAM_BLOCK_SIZE);
        meta->address[i] = stash_list[i]->address;
//        logf("address %d in bucket %d, server", stash_list[i]->address, bucket_id);
    }
    for (i = count; i < ORAM_BUCKET_SIZE; i++) {
        encrypt_message(sock_write->bucket.data[meta->offset[i]], client_t.blank_data, ORAM_BLOCK_SIZE);
        if (i < ORAM_BUCKET_REAL)
            meta->address[i] = -1;
    }
    sock_ctx->type = SOCKET_WRITE_BUCKET;
    sock_write->bucket_id = bucket_id;
    sock_write->bucket.read_counter = 0;
    memset(sock_write->bucket.valid_bits, 1, sizeof(sock_write->bucket.valid_bits));
    encrypt_message(sock_write->bucket.encrypt_metadata, (unsigned char *)meta, ORAM_META_SIZE);
    sock_send_recv(ctx->sock, socket_buf, socket_buf, ORAM_SOCKET_WRITE_SIZE, ORAM_SOCKET_WRITE_SIZE_R);
}
int get_metadata_helper(int pos, unsigned char *socket_buf, oram_bucket_encrypted_metadata metadata[], access_ctx *ctx) {
    int pos_run, i;
    socket_ctx *sock_ctx = (socket_ctx *)socket_buf;
    socket_get_metadata *sock_meta = (socket_get_metadata *)sock_ctx->buf;
    socket_get_metadata_r *sock_meta_r = (socket_get_metadata_r *)sock_ctx->buf;
    sock_ctx->type = SOCKET_GET_META;
    sock_meta->pos = pos;
    sock_send_recv(ctx->sock, socket_buf, socket_buf, ORAM_SOCKET_META_SIZE,
                   ORAM_SOCKET_META_SIZE_R(client_t.oram_tree_height));
    for (i = 0, pos_run = pos; ; pos_run = (pos_run - 1) >> 1, ++i) {
        if (decrypt_message(metadata[i].encrypt_metadata,
                            sock_meta_r->metadata[i].encrypt_metadata,
                            ORAM_CRYPT_META_SIZE_DE) != 0) {
            return -1;
        }
        metadata[i].read_counter = sock_meta_r->metadata[i].read_counter;
        ctx->metadata_counter[i] = metadata[i].read_counter;
        memcpy(metadata[i].valid_bits, sock_meta_r->metadata[i].valid_bits, sizeof(metadata[0].valid_bits));
        if (pos_run == 0)
            break;
    }
    return 0;
}

int read_block_helper(int pos, int address, unsigned char socket_buf[],
                      oram_bucket_encrypted_metadata metadata[],
                      unsigned char data[], access_ctx *ctx) {
    int found = 0, i, j, pos_run, found_pos = -1;
    unsigned char xor_tem[ORAM_CRYPT_DATA_SIZE];
    socket_ctx *sock_ctx = (socket_ctx *)socket_buf;
    socket_read_block *sock_block = (socket_read_block *)sock_ctx->buf;
    socket_read_block_r *sock_block_r = (socket_read_block_r *)sock_ctx->buf;
    sock_ctx->type = SOCKET_READ_BLOCK;
    sock_block->pos = pos;
    oram_bucket_metadata *de_meta;
    for (i = 0, found = 0, pos_run = pos; ; pos_run = (pos_run - 1) >> 1, ++i) {
        de_meta = (oram_bucket_metadata *)&metadata[i].encrypt_metadata;
        if (found == 1)
            sock_block->offsets[i] = get_random_dummy(metadata[i].valid_bits, de_meta->offset);
        else {
            for (j = 0; j < ORAM_BUCKET_REAL; j++) {
                if (de_meta->address[j] == address && metadata[i].valid_bits[de_meta->offset[j]] == 1) {
                    sock_block->offsets[i] = de_meta->offset[j];
                    found = 1;
                    found_pos = i;
                    break;
                }
            }
            if (found == 0)
                sock_block->offsets[i] = get_random_dummy(metadata[i].valid_bits, de_meta->offset);
        }
//        logf("Read offset %d in bucket %d", sock_block->offsets[i], pos_run);
        if (pos_run == 0)
            break;
    }
    sock_send_recv(ctx->sock, socket_buf, socket_buf,
                   ORAM_SOCKET_BLOCK_SIZE(client_t.oram_tree_height),
                   ORAM_SOCKET_BLOCK_SIZE_R(client_t.oram_tree_height));
    for (i = 0, pos_run = pos;;pos_run = (pos_run - 1) >> 1, ++i) {
        encrypt_message_old(xor_tem, client_t.blank_data, ORAM_BLOCK_SIZE, sock_block_r->nonce[i]);
        if (i != found_pos)
            for (j = 0; j < ORAM_CRYPT_DATA_SIZE; j++)
                sock_block_r->data[j] ^= xor_tem[j];
        if (pos_run == 0)
            break;
    }
    if (found == 1) {
        memcpy(sock_block_r->data, sock_block_r->nonce[found_pos], ORAM_CRYPT_NONCE_LEN);
        decrypt_message(data, sock_block_r->data, ORAM_CRYPT_DATA_SIZE_DE);
    }
    return found;
}

int read_path(int pos, int address, unsigned char data[], access_ctx *ctx) {
    unsigned char socket_buf[ORAM_SOCKET_BUFFER];
    oram_bucket_encrypted_metadata metadata[ORAM_TREE_DEPTH];
    if (get_metadata_helper(pos, socket_buf, metadata, ctx) == -1){
        return -1;
    }
    int found = read_block_helper(pos, address, socket_buf, metadata, data, ctx);
    return found;
}

int get_access_node(int node_counter[], int choose_node[], int node_total, int *position_index) {
    int i, min_node, exist = 0;
    for (i = 1, min_node = choose_node[0], *position_index = 0;i < node_total;i++) {
        if (node_counter[choose_node[i]] < node_counter[min_node]) {
            min_node = choose_node[i];
            *position_index = i;
            exist = 1;
        }
    }
    if (!exist) {
        *position_index = get_random(node_total);
        min_node = choose_node[*position_index];
    }
    return min_node;
}

int oblivious_access(int address, oram_access_op op, unsigned char data[], access_ctx *ctx) {
    int leaf_pos, i;
    int position[client_t.backup_count];
    int position_new[client_t.backup_count];
    unsigned char read_data[ORAM_BLOCK_SIZE];
    stash_block *block;
    int choose_node[client_t.backup_count];
    int position_index;

    //Access from stash first
    block = find_edit_by_address(client_t.stash, address, op, data);
    if (block == NULL) {
        memcpy(position, client_t.position_map + address * client_t.backup_count, client_t.backup_count * sizeof(int));
        for (i = 0;i < client_t.backup_count;i++)
            choose_node[i] = client_t.position_map[address * client_t.backup_count + i] / client_t.oram_size;
        int access_node_id = get_access_node(client_t.node_access_counter, choose_node, client_t.backup_count, &position_index);
        int access_node_index = position[position_index] % client_t.oram_size;
        ctx->access_node = access_node_id;
        //Do not care if value is changed during access
        pthread_mutex_lock(&client_t.mutex_counter);
        client_t.node_access_counter[access_node_id]++;
        pthread_mutex_unlock(&client_t.mutex_counter);

        ctx->sock = sock_connect_init(&client_t.client_storage[access_node_id].server_addr, client_t.client_storage[access_node_id].addrlen);
        ctx->storage_ctx = &client_t.client_storage[access_node_id];
        if (address >= client_t.oram_size * client_t.node_count * ORAM_BUCKET_REAL)
            return -1;
        get_random_pair(client_t.node_count, client_t.backup_count, client_t.oram_size, position_new);
        leaf_pos = get_random_leaf(access_node_index, client_t.oram_size);
        if (read_path(leaf_pos, address, read_data, ctx) <= 0) {
            log_f("Access %d from error", address);
            return -1;
        }
        block = malloc(sizeof(stash_block));
        block->bucket_id = malloc(sizeof(int) * client_t.backup_count);
        block->address = address;
        memcpy(block->data, read_data, ORAM_BLOCK_SIZE);
        log_f("Access %d from Server", address);
        for (i = 0;i < client_t.backup_count;i++) {
            block->bucket_id[i] = position_new[i];
            client_t.position_map[address * client_t.backup_count + i] = position_new[i];
        }
        block->evict_count = client_t.backup_count;
        add_to_stash(client_t.stash, block);
        //DO NOT Reshuffle in the first time
        client_t.client_storage[access_node_id].round = (++client_t.client_storage[access_node_id].round) % ORAM_RESHUFFLE_RATE;
        if (client_t.client_storage[access_node_id].round == 0)
            evict_path(ctx);
        early_reshuffle(leaf_pos, ctx);
        pthread_mutex_lock(&client_t.mutex_counter);
        client_t.node_access_counter[access_node_id]--;
        pthread_mutex_unlock(&client_t.mutex_counter);
        if (op == ORAM_ACCESS_READ)
            memcpy(data, block->data, ORAM_BLOCK_SIZE);
        else
            memcpy(block->data, data, ORAM_BLOCK_SIZE);
        close(ctx->sock);
    }
    else {
        log_f("access %d from stash", address);
    }
    return 0;
}

int oram_server_init(int bucket_size, client_storage_ctx *ctx, oram_init_op op) {
    unsigned char buf[4096];
    socket_ctx *sock_ctx = (socket_ctx *)buf;
    socket_init *sock_init_ctx = (socket_init *)sock_ctx->buf;
    socket_init_r *sock_init_ctx_r = (socket_init_r *)sock_ctx->buf;
    sock_ctx->type = SOCKET_INIT;
    sock_init_ctx->size = bucket_size;
    sock_init_ctx->op = op;
    int sock = sock_connect_init(&ctx->server_addr, ctx->addrlen);
    memcpy(sock_init_ctx->storage_key, ctx->storage_key, ORAM_STORAGE_KEY_LEN);
    sock_send_recv(sock, buf, buf, ORAM_SOCKET_INIT_SIZE, ORAM_SOCKET_INIT_SIZE_R);
    log_f("Init Request to Server");
    close(sock);
    if (sock_init_ctx_r->status == SOCKET_RESPONSE_FAIL) {
        log_f("Init Fail, Msg:%s", sock_init_ctx_r->err_msg);
        return -1;
    }
    else
        return 0;
}

void evict_path(access_ctx *ctx) {
    log_f("evict path");
    unsigned char socket_buf[ORAM_SOCKET_BUFFER];
    int pos_run;
    socket_ctx *sock_ctx = (socket_ctx *)socket_buf;
    int pos = gen_reverse_lexicographic(ctx->storage_ctx->eviction_g, client_t.oram_size, client_t.oram_tree_height);
    ctx->storage_ctx->eviction_g = (ctx->storage_ctx->eviction_g + 1) % ((client_t.oram_size >> 1) + 1);
    sock_ctx->type = SOCKET_READ_BUCKET;
    for (pos_run = pos; ;pos_run = (pos_run - 1) >> 1) {
        read_bucket_to_stash(ctx, pos_run, socket_buf);
        if (pos_run == 0)
            break;
    }

    sock_ctx->type = SOCKET_WRITE_BUCKET;
    for (pos_run = pos; ;pos_run = (pos_run - 1) >> 1) {
        write_bucket_to_server(ctx, pos_run, socket_buf);
        if (pos_run == 0)
            break;
    }
}

void early_reshuffle(int pos, access_ctx *ctx) {
    int i, pos_run;
    unsigned char socket_buf[ORAM_SOCKET_BUFFER];
    for (i = 0, pos_run = pos;; pos_run = (pos_run - 1) >> 1, ++i) {
        //Notice that read counter in client is always one less
        if (ctx->metadata_counter[i] >= ORAM_BUCKET_DUMMY - 1) {
            log_f("early reshuffle %d on pos %d", pos_run, pos_run);
            read_bucket_to_stash(ctx, pos_run, socket_buf);
            write_bucket_to_server(ctx, pos_run, socket_buf);
        }
        if (pos_run == 0)
            break;
    }
}

int node_init(client_storage_ctx *ctx, oram_node_pair *args) {
    if (args->host == NULL)
        args->host = ORAM_DEFAULT_HOST;
    if (args->port == 0)
        args->port = ORAM_DEFAULT_PORT;
    ctx->eviction_g = 0;
    ctx->round = 0;
    sock_init(&ctx->server_addr, &ctx->addrlen, args->host, args->port);
    return 0;
}

int client_create(int node_count,int size_bucket, int backup, int re_init, oram_client_args *args) {
    int i, bk[backup], j, w, sock_set[node_count*backup], m;
    int address_position_map[size_bucket * node_count * backup][80];
    unsigned char socket_buf[ORAM_SOCKET_BUFFER];
    socket_ctx *sock_ctx = (socket_ctx *)socket_buf;
    socket_write_bucket *sock_write = (socket_write_bucket *)sock_ctx->buf;
    oram_bucket *bucket = &sock_write->bucket;
    oram_bucket_metadata metadata;
    client_t.args = args;
    client_t.client_storage = calloc(node_count * backup, sizeof(client_storage_ctx));
    client_t.oram_size = size_bucket;
    client_t.node_count = node_count;
    client_t.backup_count = backup;
    client_t.oram_tree_height = log(client_t.oram_size + 1)/log(2) + 1;
    client_t.position_map = calloc(size_bucket * node_count * ORAM_BUCKET_REAL * backup, sizeof(int));
    client_t.stash = malloc(sizeof(client_stash));
    //init worker
    for (i = 0;i < node_count * backup;i++)
        get_random_key(client_t.client_storage[i].storage_key);
    stash_block *block;
    bzero(client_t.stash, sizeof(client_stash));
    init_stash(client_t.stash, size_bucket * node_count * backup);

    pthread_mutex_init(&client_t.mutex_counter, NULL);
    client_t.node_access_counter = malloc(sizeof(int) * size_bucket * backup);
    //assign block to random bucket
    for (i = 0; i < size_bucket * node_count * ORAM_BUCKET_REAL;i++) {
        get_random_pair(node_count, backup, size_bucket, bk);
        for (j = 0;j < backup;j++)
            address_position_map[bk[j]][++address_position_map[bk[j]][0]] = i;
        memcpy(client_t.position_map + i * backup, bk, sizeof(int) * backup);
    }

    oram_init_op op = SOCKET_OP_CREATE;
    if (re_init == 1)
        op = SOCKET_OP_REINIT|SOCKET_OP_CREATE;
    for (i = 0;i < node_count * backup;i++) {
        node_init(&client_t.client_storage[i], &args->node_list[i]);
        if (oram_server_init(size_bucket, &client_t.client_storage[i], op) != 0)
            return -1;
    }

    for (i = 0;i < node_count * backup;i++) {
        sock_set[i] = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        int r = connect(sock_set[i], (struct sockaddr *)&client_t.client_storage[i].server_addr, client_t.client_storage[i].addrlen);
        if (r < 0)
            err("error connect to server");
    }

    //construct a new bucket
    bucket->read_counter = 0;
    memset(bucket->valid_bits, 1, sizeof(bucket->valid_bits));
    for (m = 0;m < node_count * backup;m++) {
        int base_addr = m * size_bucket;
        for (i = 0; i < size_bucket; i++) {
            get_random_permutation(ORAM_BUCKET_SIZE, metadata.offset);
            for (j = 0; j < address_position_map[base_addr + i][0] && j < ORAM_BUCKET_REAL; j++) {
                encrypt_message(bucket->data[metadata.offset[j]], client_t.blank_data, ORAM_BLOCK_SIZE);
                metadata.address[j] = address_position_map[base_addr + i][j + 1];
    //            logf("Address %d in bucket %d, server", address_position_map[i][j+1], i);
            }
            //Extra Blocks are written into stash
            for (w = ORAM_BUCKET_REAL; w < address_position_map[base_addr + i][0]; ++w) {
                block = malloc(sizeof(stash_block));
                block->bucket_id = malloc(sizeof(int) * client_t.backup_count);
                block->address = address_position_map[base_addr + i][w + 1];
                block->bucket_id[0] = base_addr + i;
                int u,v;
                for (u = 0, v = 1;u < client_t.backup_count;u++) {
                    if (client_t.position_map[block->address * client_t.backup_count + u] != block->bucket_id[0]) {
                        block->bucket_id[v++] = client_t.position_map[block->address * client_t.backup_count + u];
                    }
                }
                block->evict_count = 1;

                memcpy(block->data, client_t.blank_data, ORAM_BLOCK_SIZE);
                add_to_stash(client_t.stash, block);
    //            logf("Address %d in bucket %d, stash", address_position_map[i][j+1], i);
            }
            for (; j < ORAM_BUCKET_SIZE; j++) {
                encrypt_message(bucket->data[metadata.offset[j]], client_t.blank_data, ORAM_BLOCK_SIZE);
                if (j < ORAM_BUCKET_REAL) {
                    metadata.address[j] = -1;
                }
            }
            encrypt_message(bucket->encrypt_metadata, (unsigned char *)&metadata, ORAM_META_SIZE);
            sock_ctx->type = SOCKET_WRITE_BUCKET;
            sock_write->bucket_id = i;
            sock_send_recv(sock_set[m], socket_buf, socket_buf, ORAM_SOCKET_WRITE_SIZE, ORAM_SOCKET_WRITE_SIZE_R);
        }
    }
    log_f("Client Create");
    client_t.running = 1;
    for (i = 0;i < node_count * backup;i++) {
        close(sock_set[i]);
    }
    return 0;
}

int client_load(int re_init) {
    int i;
    int fd = open(client_t.args->load_file, O_RDONLY);
    if (fd < 0) {
        return -1;
    }
    read(fd, &client_t, sizeof(client_t));
    client_t.client_storage = calloc(client_t.node_count * client_t.backup_count, sizeof(client_storage_ctx));
    for (i = 0;i < client_t.node_count * client_t.backup_count;i++) {
        read(fd, &client_t.client_storage[i], sizeof(client_storage_ctx));
    }

    //read position map
    client_t.position_map = calloc(client_t.node_count * client_t.backup_count * client_t.oram_size * ORAM_BUCKET_REAL, sizeof(int));
    read(fd, client_t.position_map, sizeof(int) * client_t.oram_size * client_t.node_count * client_t.backup_count * ORAM_BUCKET_REAL);
    //read stash
    client_t.stash = malloc(sizeof(client_stash));
    init_stash(client_t.stash, client_t.oram_size * client_t.node_count * client_t.backup_count);

    int r = 1, total;
    read(fd, &total, sizeof(int));
    stash_block *block;
    while (r > 0 && total > 0) {
        block = malloc(sizeof(stash_block));
        r = read(fd, block, sizeof(stash_block));
        if (r > 0) {
            bzero(&block->hh, sizeof(block->hh));
            add_to_stash(client_t.stash, block);
        }
        total--;
    }
    oram_init_op op = SOCKET_OP_LOAD;
    if (re_init == 1)
        op |= SOCKET_OP_REINIT;
    for (i = 0;i < client_t.node_count * client_t.backup_count;i++) {
        if (oram_server_init(0, &client_t.client_storage[i], op) != 0)
            return -1;
    }
    close(fd);
    return 0;
}

void client_save(int exit) {
    int i;
    client_storage_ctx *sto_ctx =client_t.client_storage;
    if (exit) {
        for (i = 0;i < client_t.node_count * client_t.backup_count;i++)
            oram_server_init(0, &client_t.client_storage[i], SOCKET_OP_SAVE);
    }
    int fd = open(client_t.args->save_file, O_WRONLY|O_CREAT, S_IRUSR|S_IWUSR);
    if (fd < 0) {
        return;
    }
    write(fd, &client_t, sizeof(client_t));
    for (i = 0;i < client_t.node_count * client_t.backup_count;i++)
        write(fd, &sto_ctx[i], sizeof(client_storage_ctx));
    write(fd, client_t.position_map, sizeof(int) * client_t.node_count \
                                     * client_t.oram_size * client_t.backup_count \
                                     * ORAM_BUCKET_REAL);
    stash_block *s = client_t.stash->address_to_stash;
    int total = 0;
    for (;s != NULL;s = s->hh.next)
        total++;
    write(fd, &total, sizeof(int));
    for (s = client_t.stash->address_to_stash;s != NULL;s = s->hh.next)
        write(fd, s, sizeof(stash_block));
    close(fd);
}

int listen_accept(oram_client_args *args) {
    int i;
    if (sock_init_byhost(&client_t.server_addr, &client_t.addrlen,&client_t.server_sock, args->host, args->port, 1) < 0)
        return -1;

    //Init oram_request_queue
    client_t.queue = malloc(sizeof(oram_request_queue));
    client_t.queue->queue_hash = NULL;
    client_t.queue->queue_list = NULL;
    client_ctx *client = &client_t;
    sem_init(&client_t.queue->queue_semaphore, 0, 0);
    pthread_mutex_init(&client_t.queue->queue_mutex, NULL);

    client_t.worker_id = calloc(args->worker, sizeof(pthread_t));
    for (i = 0;i < args->worker;i++) {
        pthread_create(&client_t.worker_id[i], NULL, worker_func, (void *)&client_t);
    }

    struct sockaddr_in tem_addr;
    socklen_t tem_addrlen;
    log_f("Midware Started.");
    while (client_t.running) {
        int sock_recv = accept(client_t.server_sock, (struct sockaddr *)&tem_addr, &tem_addrlen);
        add_to_queue(sock_recv, client_t.queue);
    }
    return 0;
}

void * worker_func(void *args) {
    client_ctx *ctx = (client_ctx *)args;
    oram_request_queue_block *queue_block;
    oram_request_queue_block *call_back_list;
    unsigned char buf[ORAM_SOCKET_BUFFER];
    socket_access_r *access_r = (socket_access_r *)buf;
    while (ctx->running) {
        sem_wait(&ctx->queue->queue_semaphore);
        pthread_mutex_lock(&ctx->queue->queue_mutex);
        queue_block = ctx->queue->queue_list;
        if (queue_block != NULL)
            LL_DELETE(ctx->queue->queue_list, queue_block);
        pthread_mutex_unlock(&ctx->queue->queue_mutex);
        access_ctx access_t;
        access_t.queue_block = queue_block;
        oblivious_access(queue_block->address, queue_block->op, queue_block->data, &access_t);
        access_r->address = queue_block->address;
        memcpy(access_r->data, queue_block->data, ORAM_BLOCK_SIZE);
        access_r->status = SOCKET_RESPONSE_SUCCESS;
        sock_standard_send(queue_block->sock, buf, ORAM_SOCKET_ACCESS_SIZE_R);
        //TODO if lock fail ?
        if (stash_block_lock(client_t.stash, queue_block->address, ORAM_LOCK_READ) == 0) {
            err("error in locking stash");
        }
        for (call_back_list = queue_block->call_back_list; call_back_list != NULL;call_back_list = call_back_list->next_l) {
            find_edit_by_address(client_t.stash, call_back_list->address, call_back_list->op, call_back_list->data);
        }
        stash_block_unlock(client_t.stash, queue_block->address);
        pthread_mutex_lock(&ctx->queue->queue_mutex);
        HASH_DEL(ctx->queue->queue_hash, queue_block);
        pthread_mutex_unlock(&ctx->queue->queue_mutex);
    }
    //process access address

}

int add_to_queue(int sock, oram_request_queue *queue) {
    socket_access access_buffer;
    int r = recv(sock, &access_buffer, ORAM_SOCKET_ACCESS_SIZE, 0);
    oram_request_queue_block *block = malloc(sizeof(oram_request_queue_block));
    oram_request_queue_block *call_back_head;
    block->address = access_buffer.address;
    block->op = access_buffer.op;
    block->sock = sock;
    block->next_l = NULL;
    if (block->op == ORAM_ACCESS_WRITE)
        memcpy(block->data, access_buffer.data, ORAM_BLOCK_SIZE);

    pthread_mutex_lock(&queue->queue_mutex);
    HASH_FIND_INT(queue->queue_hash, &block->address, call_back_head);

    //Block request and add request to callback list
    if (call_back_head) {
        LL_APPEND(call_back_head->call_back_list, block);
        pthread_mutex_unlock(&queue->queue_mutex);
    } else {
        HASH_ADD_INT(queue->queue_hash, address, block);
        LL_PREPEND(queue->queue_list, block);
        pthread_mutex_unlock(&queue->queue_mutex);
        sem_post(&queue->queue_semaphore);
    }
}

int client_access(int address, oram_access_op op,unsigned char data[], oram_node_pair *pair) {
    unsigned char buf[ORAM_SOCKET_BUFFER];
    socket_access *access_sock = (socket_access *)buf;
    socket_access_r *access_sock_r = (socket_access_r *)buf;
    access_sock->address = address;
    access_sock->op = op;
    memcpy(access_sock->data, data, ORAM_BLOCK_SIZE);
    int sock;
    struct sockaddr_in tem_addr;
    socklen_t len;
    sock_init_byhost(&tem_addr, &len, &sock, pair->host, pair->port, 0);
    sock_send_recv(sock, buf, buf, ORAM_SOCKET_ACCESS_SIZE, ORAM_SOCKET_ACCESS_SIZE_R);
    memcpy(data, access_sock_r->data, ORAM_BLOCK_SIZE);
    return 0;
}