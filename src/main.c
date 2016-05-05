//
// Created by jyjia on 2016/4/29.
//

#include <signal.h>
#include "args.h"
#include "client.h"
#include "server.h"
#include "daemon.h"


static server_ctx sv_ctx;
static void sig_handler(int signo) {
    if (signo == SIGINT)
        exit(1);  // for gprof
    else
        server_stop(&sv_ctx);
}

int main (int argc, char* argv[]) {
    int f[2000];
    oram_args_t *args = malloc(sizeof(oram_args_t));
    args_parse(args, argc, argv);
    crypt_init();
    signal(SIGINT, sig_handler);
    signal(SIGTERM, sig_handler);
    if (args->mode == ORAM_MODE_SERVER) {
        if (args->daemon == ORAM_DAEMON_STOP) {
            if (daemon_stop(args) != 0) {
                errf("cannot stop");
                return -1;
            }
            return 0;
        }
        else if (args->daemon == ORAM_DAEMON_RESTART) {
            if (daemon_stop(args) != 0) {
                errf("cannot stop");
                return -1;
            }
            if (daemon_start(args) != 0) {
                errf("cannot start");
                return -1;
            }
        }
        else if (args->daemon == ORAM_DAEMON_START) {
            if (daemon_start(args) != 0) {
                errf("cannot start");
                return -1;
            }
        }
        server_run(args, &sv_ctx);
    }
    else {
        client_ctx ctx;
        client_init(&ctx, 50, args);
        unsigned char data[ORAM_BLOCK_SIZE];
        int m = 1;
        for(m = 0;m < 100;m++) {
            data[0] = m;
            access(m, ORAM_ACCESS_WRITE, data, &ctx);
        }
        for(m = 0;m < 100;m++) {
            access(m, ORAM_ACCESS_READ, data, &ctx);
            f[m] = data[0];
        }
        for (m = 0;m < 100;m++)
            assert(f[m] == m);
    }
    return 0;
}