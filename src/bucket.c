//
// Created by jyjia on 2016/4/30.
//

#include <stdio.h>
#include <fcntl.h>
#include "bucket.h"

int read_bucket_from_file(int bucket_id, storage_ctx *ctx) {
    char filename[50];
    sprintf(filename, ORAM_FILE_FORMAT, bucket_id);
    int fd = open(filename, O_RDONLY);
    if (fd < 0) {
        return -1;
    }
    ctx->bucket_list[bucket_id] = malloc(sizeof(oram_bucket));
    read(fd, (void *)ctx->bucket_list[bucket_id], sizeof(oram_bucket));
    close(fd);
    return 0;
}

int write_bucket_to_file(int bucket_id, storage_ctx *ctx, int remain_in_mem) {
    char filename[50];
    sprintf(filename, ORAM_FILE_FORMAT, bucket_id);
    int fd = open(filename, O_WRONLY | O_CREATE, S_IRUSR|S_IWUSR);
    write(fd, (void *)ctx->bucket_list[bucket_id], sizeof(oram_bucket));
    if (!remain_in_mem) {
        free(ctx->bucket_list[bucket_id]);
        ctx->bucket_list[bucket_id] = 0;
    }
    close(fd);
    return 0;
}

int flush_buckets(storage_ctx *ctx, int remain_in_mem) {
    int i = 0;
    for (; i < ctx->size; i++) {
        if (ctx->bucket_list[i] != 0)
            write_bucket_to_file(i, ctx, remain_in_mem);
    }
    return 0;
}

oram_bucket* get_bucket(int bucket_id, storage_ctx *ctx) {
    if (ctx->bucket_list[bucket_id] == 0)
        read_bucket_from_file(bucket_id, ctx);
    return ctx->bucket_list[bucket_id];
}