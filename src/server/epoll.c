#include "server.h"

#include <arpa/inet.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/types.h>

// epoll server implementation.
#if !USE_IO_URING

int async_server_init(const Server *server, AsyncCtx *ctx, Client *client) {
    int ret = 0;

    bzero(ctx, sizeof(AsyncCtx));

    ret = epoll_create(EPOLL_MAX_EVENTS);

    if (ret < 0) {
        return ret;
    }

    ctx->epoll_fd = ret;

    // Setup server fd.
    ctx->ev.events = EPOLLIN;
    ctx->ev.data.fd = server->fd;

    if ((ret = epoll_ctl(ctx->epoll_fd, EPOLL_CTL_ADD, server->fd, &ctx->ev)) <
        0) {
        return ret;
    }

    return ret;
}

int async_server_poll(const Server *server, AsyncCtx *ctx, Client *client) {
    int ret = 0;
    int new_events =
        epoll_wait(ctx->epoll_fd, ctx->events, EPOLL_MAX_EVENTS, -1);

    if (new_events < 0) {
        return new_events;
    }

    for (int i = 0; i < new_events; i++) {
        const struct epoll_event *event = &ctx->events[i];

        // Accept new connection.
        if (event->data.fd == server->fd) {
            unsigned int client_len = sizeof(client->addr);
            const int client_fd =
                accept4(server->fd, (struct sockaddr *)&client->addr,
                        &client_len, SOCK_NONBLOCK);

            // Make client fd non-blocking
            if (client_fd < 0) {
                fprintf(stderr, "Failed to set client fd to non-blocking: %s\n",
                        strerror(client_fd));

                exit(1);

                continue;
            }

            ctx->ev.events = EPOLLIN | EPOLLET;
            ctx->ev.data.fd = client_fd;

            if ((ret = epoll_ctl(ctx->epoll_fd, EPOLL_CTL_ADD, client_fd,
                                 &ctx->ev)) < 0) {

                fprintf(stderr, "Failed to add client fd to epoll: %s\n",
                        strerror(ret));

                close(client_fd);
            }

            continue;
        }

        // Data to read.
        if (event->events & EPOLLIN) {
            ProtocolRequest req;

            int bytes_received =
                recv(event->data.fd, &req, PROTOCOL_REQ_SIZE, 0);

            if (bytes_received == 0) {
                goto close;
            }

            if (bytes_received < 0) {
                printf("Failed to read from client: %s\n",
                       strerror(bytes_received));
                goto close;
            }

            if (bytes_received != PROTOCOL_REQ_SIZE) {
                printf("Invalid request size: %d\n", bytes_received);
                goto close;
            }

            protocol_request_to_le(&req);

            protocol_debug_print_request(&req);

            ProtocolResponse resp;
            bzero(&resp, sizeof(ProtocolResponse));

            resp.answer = 0x69;

            protocol_response_to_be(&resp);

            send(event->data.fd, &resp, PROTOCOL_RES_SIZE, 0);
        close:
            epoll_ctl(ctx->epoll_fd, EPOLL_CTL_DEL, event->data.fd, NULL);
            close(event->data.fd);
            printf("Closed fd %d\n", event->data.fd);
            continue;
        }

        printf("Unknown event: %d\n", event->events);
    }

    return 0;
}

int async_server_exit(const Server *server, AsyncCtx *ctx, Client *client) {
    return close(ctx->epoll_fd) + close(server->fd);
}

#endif