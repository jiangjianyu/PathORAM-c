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

int sock_standard_send(int sock, unsigned char send_msg[], int len) {
    int r = send(sock, send_msg, len, 0);
    if (r <= 0)
        return -1;
    return 0;
}

int sock_standard_recv(int sock, unsigned char recv_msg[], int sock_len) {
    int total = 0, r = 0;
    while (total < sock_len) {
        r = recv(sock, recv_msg + total, ORAM_SOCKET_BUFFER, 0);
        total += r;
    }
    return 0;
}

//recv left package
int sock_recv_add(int sock, unsigned char recv_msg[], int now, int len) {
    //TODO exit when recv too many times
    int total = now;
    while (total < len) {
        logf("recv now %d, total %d", now, len);
        now = recv(sock, recv_msg + total, ORAM_SOCKET_BUFFER, 0);
        if (now == 0) {
            logf("wrong package");
            return -1;
        }
        total += now;
        logf("recv one more time");
    }
    return 0;
}

int sock_send_recv(int sock, unsigned char send_msg[], unsigned char recv_msg[],
                   int send_len, int recv_len) {
    sock_standard_send(sock, send_msg, send_len);
    //TODO stop when timeout, or return status is wrong
    sock_standard_recv(sock, recv_msg, recv_len);
    return 0;
}