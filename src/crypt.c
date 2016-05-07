//
// Created by jyjia on 2016/4/29.
//


#include "crypt.h"
#include "log.h"

void gen_crypt_pair(crypt_ctx *ctx) {
    randombytes_buf(ctx->nonce, ORAM_CRYPT_NONCE_LEN);
//    randombytes_buf(ctx->key, ORAM_CRYPT_KEY_LEN);
    memcpy(ctx->key, cr_ctx->key, ORAM_CRYPT_KEY_LEN);
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

int get_random_but(int range, int but) {
    int random = randombytes_uniform(range);
    if (random == but)
        return random + 1;
    return random;
}

void encrypt_message_old(unsigned char *ciphertext, unsigned char *message, int len, unsigned char nonce[]) {
    crypto_secretbox_easy(ciphertext + ORAM_CRYPT_NONCE_LEN, message, len, nonce, cr_ctx->key);
}

int decrypt_message_old(unsigned char* message, unsigned char *ciphertext, int cipher_len, unsigned char nonce[]) {
    //Nonce in ciphertext has been xor, use real nonce instead.
    if (crypto_secretbox_open_easy(message, ciphertext + ORAM_CRYPT_NONCE_LEN, cipher_len, nonce, cr_ctx->key) != 0) {
        logf("Decrypting Message Error, Maybe Forged!!");
        return -1;
    }
    return 0;
}

void encrypt_message(unsigned char *ciphertext, unsigned char *message, int len) {
    crypt_ctx ctx;
    gen_crypt_pair(&ctx);
    crypto_secretbox_easy(ciphertext + ORAM_CRYPT_NONCE_LEN, message, len, ctx.nonce, ctx.key);
    memcpy(ciphertext, ctx.nonce, ORAM_CRYPT_NONCE_LEN);
}

int decrypt_message(unsigned char* message, unsigned char *ciphertext, int cipher_len) {
    if (crypto_secretbox_open_easy(message, ciphertext + ORAM_CRYPT_NONCE_LEN, cipher_len, ciphertext, cr_ctx->key) != 0) {
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