//
// Created by jyjia on 2016/4/30.
//

#include <time.h>
#include "log.h"
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