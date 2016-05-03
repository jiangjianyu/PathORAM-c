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
        access(25, ORAM_ACCESS_READ, data, &ctx);
    }
    return 0;
}