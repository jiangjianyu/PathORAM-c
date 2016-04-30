//
// Created by jyjia on 2016/4/29.
//

#ifndef PATHORAM_ORAM_H
#define PATHORAM_ORAM_H

#define ORAM_TOTAL_BUCKET 100000
#define ORAM_ADDRESS_SIZE 10
#define ORAM_TREE_DEPTH 10

#define ORAM_BUCKET_REAL 20
#define ORAM_BUCKET_DUMMY 20
#define ORAM_BUCKET_SIZE 40

#define ORAM_BLOCK_SIZE 4096

typedef enum {
    ORAM_ACCESS_READ = 0,
    ORAM_ACCESS_WRITE = 1
} oram_access_op;

#endif //PATHORAM_ORAM_H
