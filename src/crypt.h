//
// Created by jyjia on 2016/4/29.
//

#ifndef PATHORAM_CRYPT_H
#define PATHORAM_CRYPT_H

#include "libsodium/libsodium.h"
#include <stdio.h>
#include <stdlib.h>
#include "oram.h"

#define ORAM_CRYPT_KEY_LEN crypto_aead_aes256gcm_KEYBYTES
#define ORAM_CRYPT_NONCE_LEN crypto_secretbox_NONCEBYTES
#define ORAM_CRYPT_OVERHEAD crypto_secretbox_MACBYTES
#define KEY "PATHORAM"
#define get_random(range) randombytes_uniform(range)

typedef struct {
    unsigned char nonce[crypto_secretbox_NONCEBYTES];
    unsigned char key[crypto_secretbox_KEYBYTES];
} crypt_ctx;

crypt_ctx *cr_ctx;

int crypt_init();

int get_random(int range);

int* get_random_permutation(int len);

char* get_random_str(int len);

void encrypt_message(unsigned char *ciphertext, unsigned char *message, int len);

int decrypt_message(unsigned char* message, unsigned char *ciphertext, int cipher_len);

#endif //PATHORAM_CRYPT_H
