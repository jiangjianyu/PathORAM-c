//
// Created by jyjia on 2016/4/29.
//

#include "crypt.h"

int crypt_init() {
    cr_ctx = new malloc(sizeof(crypt_ctx));
    bzero(cr_ctx, sizeof(crypt_ctx));
    sprintf(cr_ctx->key, KEY);
    randombytes_buf(cr_ctx->nonce, sizeof(nonce));
}

void encrypt_message_default(unsigned char *ciphertext, unsigned char *message, int len) {
    crypto_secretbox_easy(ciphertext, message, len, cr_ctx->nonce, cr_ctx->key);
}

crypt_ctx* encrypt_message_gen(unsigned char *ciphertext, unsigned char *message, int len) {
    crypt_ctx *ctx = malloc(sizeof(crypt_ctx));
    randombytes_buf(cr_ctx->nonce, sizeof(nonce));
    randombytes_buf(cr_ctx->key, sizeof(key));
    crypto_secretbox_easy(ciphertext, message, len, ctx->nonce, ctx.key);
    return ctx;
}

int decrypt_message_default(unsigned char* message, unsigned char *ciphertext, int cipher_len) {
    return crypto_secretbox_open_easy(message, ciphertext, cipher_len, cr_ctx->nonce, cr_ctx->key);
}

int decrypt_message_gen(unsigned char* message, unsigned char *ciphertext, int cipher_len, crypt_ctx *ctx) {
    return crypto_secretbox_open_easy(message, ciphertext, cipher_len, ctx->nonce, ctx->key);
}

typedef struct {
    int random;
    int no;
} two_random;

int cmp(const void *a, const void *b) {
    return ((two_random *)a)->random <= ((two_random *)b)->random;
}

int get_random_permutation(int len, int random[]) {
    two_random random_list[len];
    int i;
    for (i = 0; i < len; i++) {
        random_list[i].random = randombytes_uniform(len << 7);
        random_list[i].no = i;
    }
    qsort(random_list, len, sizeof(two_random), cmp);
    for (i = 0; i < len; i++)
        random[i] = random_list[i].no;
    return 0;
}