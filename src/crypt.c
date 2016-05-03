//
// Created by jyjia on 2016/4/29.
//


#include "crypt.h"
#include "log.h"

void gen_crypt_pair(crypt_ctx *ctx) {
    randombytes_buf(ctx->nonce, ORAM_CRYPT_NONCE_LEN);
    randombytes_buf(ctx->key, ORAM_CRYPT_KEY_LEN);
}

void crypt_init() {
    cr_ctx = malloc(sizeof(crypt_ctx));
    bzero(cr_ctx, sizeof(crypt_ctx));
    sprintf(cr_ctx->key, KEY);
    randombytes_buf(cr_ctx->nonce, ORAM_CRYPT_NONCE_LEN);
    logf("Cryptographic Key Init");
}

int get_random(int range) {
    return randombytes_uniform(range);
}

void encrypt_message_default(unsigned char *ciphertext, unsigned char *message, int len) {
    crypto_secretbox_easy(ciphertext, message, len, cr_ctx->nonce, cr_ctx->key);
}

void encrypt_message_gen(unsigned char *ciphertext, unsigned char *message, int len, crypt_ctx *ctx) {
//    gen_crypt_pair(ctx);
    sprintf(ctx->key, "oo");
    sprintf(ctx->nonce, "00");
    crypto_secretbox_easy(ciphertext, message, len, ctx->nonce, ctx->key);
}

void encrypt_message_th(unsigned char *ciphertext, unsigned char *message, int len, crypt_ctx *ctx) {
    crypto_secretbox_easy(ciphertext, message, len, ctx->nonce, ctx->key);
}

int decrypt_message_default(unsigned char* message, unsigned char *ciphertext, int cipher_len) {
    if (crypto_secretbox_open_easy(message, ciphertext, cipher_len, cr_ctx->nonce, cr_ctx->key) != 0) {
        logf("Decrypting Message Error, Maybe Forged!!");
        return -1;
    }
    return 0;
}

int decrypt_message_gen(unsigned char* message, unsigned char *ciphertext, int cipher_len, crypt_ctx *ctx) {
    if (crypto_secretbox_open_easy(message, ciphertext, cipher_len, ctx->nonce, ctx->key) != 0) {
        logf("Decrypting Message Error, Maybe Forged!!");
        return -1;
    }
    return 0;
}

typedef struct {
    int random;
    int no;
} two_random;

int cmp(const void *a, const void *b) {
    return ((two_random *)a)->random <= ((two_random *)b)->random;
}

void get_random_permutation(int len, int permutation[]) {
    two_random random_list[len];
    int i;
    for (i = 0; i < len; i++) {
        random_list[i].random = get_random(len << 7);
        random_list[i].no = i;
    }
    qsort(random_list, len, sizeof(two_random), cmp);
    for (i = 0; i < len; i++)
        permutation[i] = (unsigned) random_list[i].no;
}