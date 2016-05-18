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
        fprintf(o, s);                                                \
        fprintf(o, "\n");                                             \
        fflush(o);                                                    \
    }                                                                 \
} while (0)

#define log_f(s...) __LOG(stdout, 0, s)
#define errf(s...) __LOG(stderr, 1, s)
#define err(s) perror_timestamp(s, __FILE__, __LINE__)

#define log_sys(s...) log_f(s)

#if ORAM_DEBUG_LEVEL >= 1
#define log_normal(s...) log_f(s)
#else
#define log_normal(s...)
#endif

#if ORAM_DEBUG_LEVEL >= 2
#define log_detail(s...) log_f(s)
#else
#define log_detail(s...)
#endif

#if ORAM_DEBUG_LEVEL >= 3
#define log_all(s...) log_f(s)
#else
#define log_all(s...)
#endif

#ifdef DEBUG
#define debugf(s...) logf(s)
#else
#define debugf(s...)
#endif

void log_timestamp(FILE *out);
void perror_timestamp(const char *msg, const char *file, int line);

#endif //PATHORAM_LOG_H
