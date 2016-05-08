//
// Created by jyjia on 2016/4/29.
//

#ifndef PATHORAM_CRYPT_H
#define PATHORAM_CRYPT_H

#include <sodium.h>
#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include "oram.h"

#define ORAM_CRYPT_KEY_LEN crypto_secretbox_KEYBYTES
#define ORAM_CRYPT_NONCE_LEN crypto_secretbox_NONCEBYTES
#define ORAM_CRYPT_OVERSIZE crypto_secretbox_MACBYTES + ORAM_CRYPT_NONCE_LEN
#define ORAM_CRYPT_OVERHEAD crypto_secretbox_MACBYTES
#define KEY "PATHORAM"
#define ORAM_STORAGE_KEY_LEN 10

typedef struct {
    unsigned char nonce[crypto_secretbox_NONCEBYTES];
    unsigned char key[crypto_secretbox_KEYBYTES];
} crypt_ctx;

crypt_ctx *cr_ctx;

void crypt_init(unsigned char key[]);

void gen_crypt_pair(crypt_ctx *ctx);

void get_random_key(char *key);

int get_random(int range);

int get_random_but(int range, int but);

void get_random_permutation(int len, int permutation[]);

void encrypt_message_old(unsigned char *ciphertext, unsigned char *message, int len, unsigned char nonce[]);

void encrypt_message(unsigned char *ciphertext, unsigned char *message, int len);

int decrypt_message_old(unsigned char* message, unsigned char *ciphertext, int cipher_len, unsigned char nonce[]);

int decrypt_message(unsigned char* message, unsigned char *ciphertext, int cipher_len);

#endif //PATHORAM_CRYPT_H
