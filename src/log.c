//
// Created by jyjia on 2016/4/30.
//

#include <time.h>

int verbose_mode;

void log_timestamp(FILE *out) {
    time_t now;
    time(&now);
    char *time_str = ctime(&now);
    time_str[strlen(time_str) - 1] = '\0';
    fprintf(out, "%s ", time_str);
}

void perror_timestamp(const char *msg, const char *file, int line) {
    log_timestamp(stderr);
    fprintf(stderr, "%s:%d ", file, line);
    perror(msg);
}

void print_hex_memory(void *mem, size_t len) {
    int i;
    unsigned char *p = (unsigned char *)mem;
    for (i = 0; i < len; i++) {
        printf("%02x ", p[i]);
        if (i % 16 == 15)
            printf("\n");
    }
    printf("\n");
}