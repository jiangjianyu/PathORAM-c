//
// Created by jyjia on 2016/4/29.
//

#include "server.h"
#include "socket.h"
int read_bucket(sock_ctx *sock_ctx, int bucket_id);

int write_bucket(sock_ctx *sock_ctx, int bucket_id);

int get_metadata(int pos, oram_bucket_metadata metadata[]);

int read_block(server_ctx *ctx, int pos, int bucket_offset_list[]);

int server_run(oram_args_t *args) {
    server_ctx *sv_ctx = malloc(sizeof(server_ctx));
    bzero(sv_ctx, sizeof(server_ctx));
    inet_aton(args->host, &sv_ctx->server_addr.sin_addr);
    sv_ctx->server_addr.sin_port = htons(args->port);
    sv_ctx->server_addr.sin_family = AF_INET;
    sv_ctx->socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    sv_ctx->buff = malloc(ORAM_SOCKET_BUFFER);

    while (sv_ctx->running == 1) {
        bzero(sv_ctx->buff, ORAM_SOCKET_BUFFER);
        recvfrom(sv_ctx->socket, sv_ctx->buff, ORAM_SOCKET_BUFFER,
                 0, (struct sockaddr *)&sv_ctx->client_addr,
                 &sv_ctx->client_addr);
        socket_ctx *sock_ctx = (socket_ctx *)sv_ctx->buff;
        switch (sock_ctx->type) {
            case SOCKET_READ_BUCKET: read_bucket();
                break;
            case SOCKET_READ_BUCKET: read_bucket();
                break;
        }

    }
}