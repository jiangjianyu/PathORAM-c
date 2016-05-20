#include "performance.h"

p_per_ctx p_ctx;

void p_get_time_now() {
    
}

void * p_func(void **args) {
    unsigned char buf[sizeof(int)];
    p_ctx.sock = socket(AF_INET, SOCK_DATAGRAM, IPPROTO_UDP);
    struct sockaddr_in addr, client_addr;
    socklen_t sock_len, client_addr_len;
    sock_len = sizeof(struct sockaddr_in);
    inet_aton(P_HOST, &addr->sin_addr);
    addr->sin_port = htons(P_PORT);
    addr->sin_family = AF_INET;
    bind(p_ctx.sock, (struct sockaddr *)addr, sock_len);
    while(1) {
        recvfrom(p_ctx.sock, buf, &client_addr, &client_addr_len);
        if (*((int *)buf) == -1) {
            sendto(p_ctx.sock, &p_ctx, &client_addr, client_addr_len);
        }
    }
}

void p_init() {
    pthread_create(&p_pid, NULL, p_func, NULL);
}