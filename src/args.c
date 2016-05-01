//
// Created by jyjia on 2016/5/1.
//

#include "args.h"
#include "log.h"
#include <unistd.h>
#include <stdlib.h>


static const char *help_message =
"usage: pathoram -c config_file -m [server|client] -b [host] -p port -d [start|stop|restart] -v\n"
"\n"
"  -h, --help            show this help message and exit\n"
"  -s start/stop/restart control pathoram process. if omitted, will run\n"
"                        in foreground\n"
"  -c config_file        path to config file\n"
"  -v                    verbose logging\n"
"  -b address            listen address\n"
"  -p port               listen port\n"
"\n";

static void print_help() {
    printf(help_message);
    exit(1);
}

void load_default_args(oram_args_t *args) {
    args->host = ORAM_DEFAULT_HOST;
    args->port = ORAM_DEFAULT_PORT;
}

void args_parse(oram_args_t *args, int argc, char **argv) {
    int ch;
    bzero(args, sizeof(oram_args_t));
    load_default_args(args);
    while ((ch = getopt(argc, argv, "b:s:p:vm:h")) != -1) {
        switch (ch) {
            case 'b': args->host = optarg;
                break;
            case 'p': args->port = atoi(optarg);
                break;
            case 's':
                if (strcmp("start", optarg) == 0)
                    args->daemon = ORAM_DAEMON_START;
                else if(strcmp("stop", optarg) == 0)
                    args->daemon = ORAM_DAEMON_STOP;
                else if(strcmp("restart", optarg) == 0)
                    args->daemon = ORAM_DAEMON_RESTART;
                else
                    errf("unknown command %s", optarg);
            case 'v': verbose_mode = 1;
                break;
            case 'm':
                if (strcmp("server", optarg) == 0)
                    args->mode = ORAM_MODE_SERVER;
                else if (strcmp("client", optarg) == 0)
                    args->mode = ORAM_MODE_CLIENT;
                else
                    errf("unknow command %s", optarg);
                break;
            case 'h': print_help();
                break;
            default:
                errf("unknown command %c", ch);
        }
    }
}

