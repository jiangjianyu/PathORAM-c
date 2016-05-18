//
// Created by jyjia on 2016/4/30.
//

#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include "bucket.h"
#include "log.h"
oram_bucket* read_bucket_from_file(int bucket_id) {
    char filename[50];
    sprintf(filename, ORAM_FILE_BUCKET_FORMAT, bucket_id);
    oram_bucket *bucket = malloc(sizeof(oram_bucket));
    int fd = open(filename, O_RDONLY);
    if (fd < 0) {
        errf("Error Reading File to Mem");
        return NULL;
    }
    read(fd, bucket, sizeof(oram_bucket));
    close(fd);
    return bucket;
}

void write_bucket_to_file(int bucket_id, storage_ctx *ctx, int remain_in_mem) {
    char filename[50];
    sprintf(filename, ORAM_FILE_BUCKET_FORMAT, bucket_id);
    int fd = open(filename, O_WRONLY | O_CREAT, S_IRUSR|S_IWUSR);
    write(fd, (void *)ctx->bucket_list[bucket_id], sizeof(oram_bucket));
    if (!remain_in_mem) {
        free(ctx->bucket_list[bucket_id]);
        ctx->bucket_list[bucket_id] = 0;
        ctx->mem_counter--;
    }
    close(fd);
    log_f("Write Bucket %d to file", bucket_id);
}

void flush_buckets(storage_ctx *ctx) {
    int i = 0;
    for (; i < ctx->size; i++) {
        if (ctx->bucket_list[i] != 0)
            write_bucket_to_file(i, ctx, 0);
    }
}

void free_server(storage_ctx *ctx) {
    int i;
    if (ctx->size == 0)
        return;
    for (i = 0;i < ctx->size; ++i)
        if (ctx->bucket_list[i] != NULL)
            free(ctx->bucket_list[i]);
    ctx->size = 0;
    ctx->mem_counter = 0;
    ctx->mem_max = 0;
    ctx->oram_tree_height = 0;
}

void evict_to_disk(storage_ctx *ctx, int but) {
    while (ctx->mem_counter >= ctx->mem_max) {
        int evict_num = get_random_but(ctx->size, but);
        if (ctx->bucket_list[evict_num] == 0)
            continue;
        write_bucket_to_file(evict_num, ctx, 0);
    }
}

oram_bucket* new_bucket() {
    oram_bucket *ne = malloc(sizeof(oram_bucket));
    ne->read_counter = 0;
    memset(ne->valid_bits, 1, sizeof(ne->valid_bits));
    return ne;
}

oram_bucket* get_bucket(int bucket_id, storage_ctx *ctx) {
    if (ctx->bucket_list[bucket_id] == 0) {
        ctx->bucket_list[bucket_id] = read_bucket_from_file(bucket_id);
        ctx->mem_counter++;
        evict_to_disk(ctx, bucket_id);
    }
    return ctx->bucket_list[bucket_id];
}
