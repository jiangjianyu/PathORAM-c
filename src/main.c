//
// Created by jyjia on 2016/4/29.
//

#include "args.h"
#include "client.h"
#include "server.h"
#include "log.h"

int main (int argc, char* argv[]) {
    oram_args_t *args = malloc(sizeof(oram_args_t));
    args_parse(args, argc, argv);
    crypt_init();
    if (args->mode == ORAM_MODE_SERVER) {
        server_run(args);
        logf("Server Started");
    }
    else {
        client_ctx ctx;
        client_init(&ctx, 10000, args);
        oram_server_init(10000, &ctx);
    }
    return 0;
}