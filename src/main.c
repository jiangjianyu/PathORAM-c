//
// Created by jyjia on 2016/4/29.
//

#include "args.h"
#include "client.h"
#include "server.h"

int main (int argc, int args[]) {
    oram_args_t *args = malloc(sizeof(oram_args_t));
    parse_config(args);
    if (args->mode == ORAM_MODE_CLIENT)
        server_run(args);
    else
        client_run(args);

    return 0;
}