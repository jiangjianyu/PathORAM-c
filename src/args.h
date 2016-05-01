//
// Created by jyjia on 2016/4/29.
//

#ifndef PATHORAM_ARGS_H
#define PATHORAM_ARGS_H

typedef enum {
    ORAM_MODE_SERVER = 0,
    ORAM_MODE_CLIENT = 1
} oram_mode;

typedef enum {
    ORAM_DAEMON_NONE = 0,
    ORAM_DAEMON_START = 1,
    ORAM_DAEMON_STOP = 2,
    ORAM_DAEMON_RESTART = 3
} oram_daemon;

typedef struct {
    int port;
    char *host;
    oram_mode mode;
    oram_daemon daemon;
} oram_args_t;

int args_parse(oram_args_t *args, int argc, char **argv);

#endif //PATHORAM_ARGS_H
