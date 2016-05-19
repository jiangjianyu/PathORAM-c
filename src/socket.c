//
// Created by jyjia on 2016/5/2.
//

#include "socket.h"
#include "log.h"

int sock_init_byhost(struct sockaddr_in *addr, socklen_t *addrlen, int *sock,
               char *host, int port, int if_bind) {
    int r = 0;
    inet_aton(host, &addr->sin_addr);
    addr->sin_port = htons(port);
    addr->sin_family = AF_INET;
    *sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    *addrlen = sizeof(struct sockaddr_in);
    if (if_bind) {
        if (bind(*sock, (struct sockaddr *) addr, *addrlen) < 0) {
            err("bind error");
            return -1;
        }
        if (listen(*sock, ORAM_SOCKET_BACKLOG) < 0) {
            err("listen error");
            return -1;
        }
    }
    else {
        if (connect(*sock, (struct sockaddr *) addr, *addrlen) < 0) {
            err("connect error");
            return -1;
        }
    }
    return 0;
}

int sock_connect_init(struct sockaddr_in *addr, socklen_t addrlen) {
    int return_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (connect(return_sock, (struct sockaddr *)addr, addrlen) < 0) {
        err("connect error");
        return -1;
    }
    return return_sock;
}

void sock_init(struct sockaddr_in *addr, socklen_t *addrlen, char *host, int port) {
    inet_aton(host, &addr->sin_addr);
    addr->sin_port = htons(port);
    addr->sin_family = AF_INET;
    *addrlen = sizeof(struct sockaddr_in);
}

int sock_standard_send(int sock, unsigned char send_msg[], int len) {
    if (len <= 0)
        return 0;
    ssize_t r;
    int retry = 0;
    struct timeval tv;
    tv.tv_sec = 5;
    tv.tv_usec = 0;
//    setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));
    while (1) {
        if (retry >= 5) {
            log_f("send connection timeout, exit");
            return -1;
        }
        r = send(sock, send_msg, len, 0);
        if (r < 0) {
            if (errno == EWOULDBLOCK) {
                log_f("timeout retry %d", retry);
                continue;
            }
            err("connection error");
            return -1;
        } else if (r == 0) {
            err("connection close");
            return -1;
        } else {
            return 0;
        }
    }
}

int sock_standard_recv(int sock, unsigned char recv_msg[], int sock_len) {
    int total = 0, r = 0 ,retry = 0;
    struct timeval tv;
    tv.tv_sec = 5;
    tv.tv_usec = 0;
//    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
    while (total < sock_len) {
        if (retry >= 5) {
            log_f("connection timeout, exit");
            return -1;
        }
        r = recv(sock, recv_msg + total, sock_len, 0);
        if (r < 0) {
            if (errno == EWOULDBLOCK) {
                log_f("connection timeout, retry %d", retry);
                retry++;
                continue;
            }
            err("connection error");
            return -1;
        } else if (r == 0) {
            err("connection close");
            return -1;
        }
        total += r;
    }
    return 0;
}

//recv left package
int sock_recv_add(int sock, unsigned char recv_msg[], int now, int len) {
    struct timeval tv;
    tv.tv_sec = 1;
    tv.tv_usec = 0;
    int retry = 0;
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
    int total = now;
    while (total < len) {
        if (retry >= 5) {
            log_f("connection timeout, exit");
            return -1;
        }
        now = recv(sock, recv_msg + total, ORAM_SOCKET_BUFFER_MAX, 0);
        if (now < 0) {
            if (errno == EWOULDBLOCK) {
                log_f("connection timeout, retry %d", retry);
                retry++;
                continue;
            }
            err("connection error");
            return -1;
        } else if (now == 0) {
            err("connection close");
            return -1;
        }
        total += now;
    }
    return 0;
}

int sock_send_recv(int sock, unsigned char send_msg[], unsigned char recv_msg[],
                   int send_len, int recv_len) {
    sock_standard_send(sock, send_msg, send_len);
    sock_standard_recv(sock, recv_msg, recv_len);
    return 0;
}