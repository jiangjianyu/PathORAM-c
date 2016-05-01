//
// Created by jyjia on 2016/4/30.
//

#ifndef PATHORAM_LOG_H
#define PATHORAM_LOG_H

#include <string.h>
#include <stdio.h>
#include <errno.h>

extern int verbose_mode;

/*
   err:    same as perror but with timestamp
   errf:   same as printf to stderr with timestamp and \n
   logf:   same as printf to stdout with timestamp and \n,
           and only enabled when verbose is on
   debugf: same as logf but only compiles with DEBUG flag
*/

#define __LOG(o, not_stderr, s...) do {                               \
    if (not_stderr || verbose_mode) {                                 \
        log_timestamp(o);                                             \
        fprintf(o, s);                                                \
        fprintf(o, "\n");                                             \
        fflush(o);                                                    \
    }                                                                 \
} while (0)

#define logf(s...) __LOG(stdout, 0, s)
#define errf(s...) __LOG(stderr, 1, s)
#define err(s) perror_timestamp(s, __FILE__, __LINE__)

#ifdef DEBUG
#define debugf(s...) logf(s)
#else
#define debugf(s...)
#endif

void log_timestamp(FILE *out);
void perror_timestamp(const char *msg, const char *file, int line);

#endif //PATHORAM_LOG_H
