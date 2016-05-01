//
// Created by jyjia on 2016/4/29.
//

#include "args.h"
#include "client.h"
#include "server.h"

int main (int argc, int argv[]) {
    oram_args_t *args = malloc(sizeof(oram_args_t));
    args_parse(args, argc, argv);
    if (args->mode == ORAM_MODE_CLIENT)
        server_run(args);
    else {
        client_ctx ctx;
        client_init(&ctx, 10000, args);
    }
    return 0;
}