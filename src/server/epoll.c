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

    async_data_pack((AsyncData *)&ctx->ev.data.u64, server->fd, 0, ACCEPT);

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

        AsyncOperation type;
        int32_t fd1;
        int32_t fd2;

        async_data_unpack((AsyncData *)&event->data.u64, &fd1, &fd2, &type);

        printf("Event: %d from: %d to: %d\n", type, fd1, fd2);

        switch (type) {

        // Accept new connection.
        case ACCEPT:
            unsigned int client_len = sizeof(client->addr);

            const int client_fd = accept4(fd1, (struct sockaddr *)&client->addr,
                                          &client_len, SOCK_NONBLOCK);

            if (client_fd < 0) {
                fprintf(stderr, "Failed to set accept client: %s\n",
                        strerror(client_fd));

                continue;
            }

            ctx->ev.events = EPOLLIN | EPOLLET;

            async_data_pack((AsyncData *)&ctx->ev.data.u64, client_fd, 0, READ);

            if ((ret = epoll_ctl(ctx->epoll_fd, EPOLL_CTL_ADD, client_fd,
                                 &ctx->ev)) < 0) {

                fprintf(stderr, "Failed to add client %d to epoll: %s\n",
                        client_fd, strerror(ret));
                close(client_fd);
            }

            break;
        // Read from connection
        case READ:
            ProtocolRequest req;

            const int bytes_received = recv(fd1, &req, PROTOCOL_REQ_SIZE, 0);

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

            send(fd1, &resp, PROTOCOL_RES_SIZE, 0);
        close:
            epoll_ctl(ctx->epoll_fd, EPOLL_CTL_DEL, fd1, NULL);
            close(fd1);
            printf("Closed fd %d\n", fd1);
            break;
        // Respond to connection
        case EVENT_FD:
            break;
        default:
            printf("Unknown event: %d type: %d\n", event->events, type);
            break;
        }
    }

    return 0;
}

int async_server_exit(const Server *server, AsyncCtx *ctx, Client *client) {
    return close(ctx->epoll_fd) + close(server->fd);
}

#endif