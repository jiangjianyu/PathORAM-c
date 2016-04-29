//
// Created by jyjia on 2016/4/29.
//

int crypt_init() {
    cr_ctx = new malloc(sizeof(crypt_ctx));
    bzero(cr_ctx, sizeof(crypt_ctx));
    sodium_init();
    if (crypto_aead_aes256gcm_is_available() == 0) {
        abort(); /* Not available on this CPU */
    }
    sprintf(cr_ctx->key, KEY);
    randombytes_buf(cr_ctx->nonce, sizeof(nonce));
}

void encrypt_message(unsigned char *ciphertext, unsigned char *message, int len) {
    crypto_secretbox_easy(ciphertext, message, len, cr_ctx->nonce, cr_ctx->key);
}

int decrypt(unsigned char* message, unsigned char *ciphertext, int cipher_len) {
    return crypto_secretbox_open_easy(message, ciphertext, cipher_len, cr_ctx->nonce, cr_ctx->key);
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
        random_list[i].random = randombutes_uniform(len << 7);
        random_list[i].no = i;
    }
    qsort(random_list, len, sizeof(two_random), cmp);
    for (i = 0; i < len; i++)
        random[i] = random_list[i].no;
    return 0;
}