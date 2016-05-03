//
// Created by jyjia on 2016/5/2.
//

#include "socket.h"

void sock_init(struct sockaddr_in *addr, socklen_t *addrlen, int *sock,
               char *host, int port, int if_bind) {
    inet_aton(host, &addr->sin_addr);
    addr->sin_port = htons(port);
    addr->sin_family = AF_INET;
    *sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    *addrlen = sizeof(struct sockaddr_in);
    if (if_bind)
        bind(*sock, (struct sockaddr *)addr, *addrlen);
}