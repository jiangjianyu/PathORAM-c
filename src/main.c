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
    int f[4000];
    oram_args_t *args = malloc(sizeof(oram_args_t));
    args_parse(args, argc, argv);
    signal(SIGINT, sig_handler);
    signal(SIGTERM, sig_handler);
    if (args->mode == ORAM_MODE_SERVER) {
        unsigned char k[ORAM_CRYPT_KEY_LEN];
        sprintf(k, "ORAM");
        crypt_init(k);
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
    else if (args->mode == ORAM_MODE_MIDWARE) {
        oram_client_args ar;
//        if (client_init(&ctx, &ar) < 0)
//            return -1;
        sprintf(ar.key, "ORAM");
        ar.host = "127.0.0.1";
        ar.port = 30000;
        ar.worker = 1;
        ar.node_list = calloc(4 ,sizeof(oram_node_pair));
        ar.node_list[0].host = "127.0.0.1";
        ar.node_list[1].host = "127.0.0.1";
        ar.node_list[2].host = "127.0.0.1";
        ar.node_list[3].host = "127.0.0.1";
        ar.node_list[0].port = 30001;
        ar.node_list[1].port = 30002;
        ar.node_list[2].port = 30003;
        ar.node_list[3].port = 30004;
        crypt_init(ar.key);
        if (client_create(2, 100, 2, 1, &ar) < 0)
            return -1;
        listen_accept(&ar);
//        if (client_load(&ctx, 1) < 0)
//            return -1;
    }
    else if (args->mode == ORAM_MODE_CLIENT) {
        int i;
        unsigned char data[ORAM_BLOCK_SIZE];
        oram_node_pair pair;
        pair.host = "127.0.0.1";
        pair.port = 30000;
        for (i = 0;i < 4000;i++) {
            data[0] = i % 256;
            client_access(i, ORAM_ACCESS_WRITE, data, &pair);
        }
        for (i = 0;i < 4000;i++) {
            client_access(i, ORAM_ACCESS_READ, data, &pair);
            f[i] = data[0];
        }
        for (i = 0;i < 4000;i++) {
            log_f("assert %d == %d, bool %d", i, f[i], f[i] == i % 256);
            assert(f[i] == i % 256);
        }
    }
    return 0;
}