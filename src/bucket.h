//
// Created by jyjia on 2016/4/29.
//

#ifndef PATHORAM_BUCKET_H
#define PATHORAM_BUCKET_H

#include "crypt.h"
#include "bucket.h"

#define ORAM_FILE_FORMAT "ORAM_BUCKET_%d.bucket"

typedef struct {
    unsigned int address[ORAM_BUCKET_REAL];
    unsigned int offset[ORAM_BUCKET_SIZE];
    crypt_ctx encrypt_par;
} oram_bucket_metadata;

#define ORAM_CRYPT_META_SIZE sizeof(oram_bucket_metadata) + ORAM_CRYPT_OVERHEAD
#define ORAM_CRYPT_DATA_SIZE ORAM_BLOCK_SIZE + ORAM_CRYPT_OVERHEAD
typedef struct {
    unsigned int read_counter;
    _Bool valid_bits[ORAM_BUCKET_SIZE];
    unsigned char encrypt_metadata[ORAM_CRYPT_META_SIZE];
    unsigned char data[ORAM_BUCKET_SIZE][ORAM_CRYPT_DATA_SIZE];
} oram_bucket;

#define ORAM_META_SIZE sizeof(oram_bucket_metadata)


typedef struct {
    unsigned int read_counter;
    _Bool valid_bits[ORAM_BUCKET_SIZE];
    unsigned char encrypt_metadata[ORAM_CRYPT_META_SIZE];
} oram_bucket_encrypted_metadata;


#define ORAM_CRYPT_META_TOTAL_SIZE sizeof(oram_bucket_encrypted_metadata) + ORAM_CRYPT_OVERHEAD
#define ORAM_CRYPT_DATA_SIZE ORAM_BLOCK_SIZE + ORAM_CRYPT_OVERHEAD

typedef struct {
    int mem_counter;
    int size;
    oram_bucket *bucket_list[ORAM_TOTAL_BUCKET];
} storage_ctx;

void read_bucket_from_file(int bucket_id, storage_ctx *ctx);

void write_bucket_to_file(int bucket_id, storage_ctx *ctx, int remain_in_mem);

void flush_buckets(storage_ctx *ctx, int remain_in_mem);

oram_bucket* get_bucket(int bucket_id, storage_ctx *ctx);


#endif //PATHORAM_BUCKET_H