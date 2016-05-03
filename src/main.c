//
// Created by jyjia on 2016/4/29.
//

#include "args.h"
#include "client.h"
#include "server.h"

int main (int argc, char* argv[]) {
    oram_args_t *args = malloc(sizeof(oram_args_t));
    args_parse(args, argc, argv);
    crypt_init();
    if (args->mode == ORAM_MODE_SERVER) {
        server_run(args);
    }
    else {
        client_ctx ctx;
        client_init(&ctx, 100, args);
        unsigned char data[ORAM_BLOCK_SIZE];
        int m = 0;
        for(m = 0;m < 5;m++) {
            data[0] = m;
            access(m, ORAM_ACCESS_WRITE, data, &ctx);
        }
        for(m = 0;m < 5;m++) {
            access(m, ORAM_ACCESS_READ, data, &ctx);
            assert(data[m] == m);
        }
    }
    return 0;
}