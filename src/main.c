//
// Created by jyjia on 2016/4/29.
//

#include <signal.h>
#include "args.h"
#include "client.h"
#include "server.h"
#include "daemon.h"
#include "performance.h"


static server_ctx sv_ctx;
static void sig_handler(int signo) {
    if (signo == SIGINT)
        exit(1);  // for gprof
    else
        server_stop(&sv_ctx);
}

void * access_func(void *args){
    int i;
    int *base = (int *)args;
    unsigned char data[ORAM_BLOCK_SIZE];
    oram_node_pair pair;
    pair.host = "127.0.0.1";
    pair.port = 30005;

    for (i = *base * 1000;i < 1000 + *base*1000;i++) {
//        data[0] = i % 256;
        client_access(i, ORAM_ACCESS_WRITE, data, &pair);
    }
    for (i = *base * 1000;i < 1000 + *base*1000;i++) {
        client_access(i, ORAM_ACCESS_READ, data, &pair);
//        f[i] = data[0];
    }
    printf("request finish all %d\n", *base);
    return NULL;
}

int main (int argc, char* argv[]) {
    int f[6000];
    oram_args_t *args = malloc(sizeof(oram_args_t));
    args_parse(args, argc, argv);
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
    else if (args->mode == ORAM_MODE_MIDWARE) {
        oram_client_args ar;
//        if (client_init(&ctx, &ar) < 0)
//            return -1;
        sprintf(ar.key, "ORAM");
        ar.host = "127.0.0.1";
        ar.port = 30005;
        ar.worker = 4;
        ar.node_list = calloc(4 ,sizeof(oram_node_pair));
        ar.node_list[0].host = "127.0.0.1";
        ar.node_list[1].host = "127.0.0.1";
        ar.node_list[2].host = "127.0.0.1";
        ar.node_list[3].host = "127.0.0.1";
        ar.node_list[0].port = 30001;
        ar.node_list[1].port = 30002;
        ar.node_list[2].port = 30003;
        ar.node_list[3].port = 30004;
        ar.save_file = "client.meta";
        ar.load_file = "client.meta";
        crypt_init(ar.key);
        if (client_create(2, 2185, 1, 1, &ar) < 0)
            return -1;
//        if (client_load(&ar, 1) < 0)
//            return -1;
        listen_accept(&ar);
    }
    else if (args->mode == ORAM_MODE_CLIENT) {
        pthread_t pid[6];
        int access_id[6];
        int m;
        p_get_performance("127.0.0.1", 30010);
        for (m = 0;m < 4;m++) {
            access_id[m] = m;
            pthread_create(&pid[m],NULL, access_func, (void*)&access_id[m]);
        }
        for (m = 0;m < 4;m++) {
            pthread_join(pid[m], NULL);
        }
        p_get_performance("127.0.0.1", 30010);
//        client_access(-1, ORAM_ACCESS_READ, data, &pair);
//        for (i = 0;i < 6000;i++) {
//            log_f("assert %d == %d, bool %d", i, f[i], f[i] == i % 256);
//            assert(f[i] == i % 256);
//        }
    }
    return 0;
}