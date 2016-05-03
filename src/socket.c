//
// Created by jyjia on 2016/5/2.
//

#include "socket.h"
#include "log.h"

void sock_init(struct sockaddr_in *addr, socklen_t *addrlen, int *sock,
               char *host, int port, int if_bind) {
    int r = 0;
    inet_aton(host, &addr->sin_addr);
    addr->sin_port = htons(port);
    addr->sin_family = AF_INET;
//    *sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    *sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    *addrlen = sizeof(struct sockaddr_in);
    if (if_bind) {
        r = bind(*sock, (struct sockaddr *) addr, *addrlen);
        if (r < 0)
            err("bind error");
        r = listen(*sock, ORAM_SOCKET_BACKLOG);
        if (r < 0)
            err("listen error");
    }
    else
        connect(*sock, (struct sockaddr *)addr, *addrlen);
}