#include "server.h"

#include <errno.h>
#include <stdio.h>

void accept_client(AsyncCtx* ctx, AsyncData* data);
void consume_request(AsyncCtx* ctx, const AsyncData* data);
void remove_client(const AsyncCtx* ctx, const AsyncData* data);

int async_server_init(const Server* server, AsyncCtx* ctx) {
    int ret = 0;

    bzero(ctx, sizeof(AsyncCtx));

    ret = epoll_create(EPOLL_MAX_EVENTS);

    if (ret < 0) {
        return ret;
    }

    // Setup server fd.
    ctx->epoll_fd = ret;
    ctx->ev.events = EPOLLIN;

    const AsyncData async_data = {.type = SERVER_ACCEPT, .fd = server->fd};
    memcpy(&ctx->ev.data.u64, &async_data, sizeof(async_data));

    if ((ret = epoll_ctl(ctx->epoll_fd, EPOLL_CTL_ADD, server->fd, &ctx->ev)) < 0) {
        return ret;
    }

    // Spawn worker pool
    ctx->scheduler = scheduler_create(8);
    ctx->worker_pool = worker_create_pool(nprocs() - 1, ctx->scheduler);

    return ret;
}

int async_server_poll(const Server* server, AsyncCtx* ctx) {
    const int new_events = epoll_wait(ctx->epoll_fd, ctx->events, EPOLL_MAX_EVENTS, -1);

    if (new_events < 0) {
        return new_events;
    }

    for (int i = 0; i < new_events; i++) {
        const struct epoll_event* event = &ctx->events[i];
        AsyncData data;
        memcpy(&data, &event->data.u64, sizeof(data));

        switch (data.type) {

        // Accept new connection.
        case SERVER_ACCEPT:
            accept_client(ctx, &data);
            break;
        // Read from connection
        case CLIENT_EVENT:
            if (event->events & EPOLLIN) {
                consume_request(ctx, &data);
            } else if (event->events & EPOLLHUP) {
                remove_client(ctx, &data);
            }

            break;

        default:
            fprintf(stderr, "Unknown event: %d type: %d\n", event->events, data.type);
            break;
        }
    }

    return 0;
}

int async_server_exit(const Server* server, const AsyncCtx* ctx) {
    // it's *very* important that the worker pool is destroyed before the scheduler
    // since the worker threads are still running and may access the scheduler.
    worker_destroy_pool(ctx->worker_pool);
    scheduler_destroy(ctx->scheduler);
    return close(ctx->epoll_fd) + close(server->fd);
}

void accept_client(AsyncCtx* ctx, AsyncData* data) {
    int ret = 0;

    struct sockaddr_in addr;
    unsigned int client_len = sizeof(addr);

    const int client_fd = accept4(data->fd, (struct sockaddr*)&addr, &client_len, SOCK_NONBLOCK);

    if (client_fd < 0) {
        fprintf(stderr, "Failed to accept client: %s from %d\n", strerror(-client_fd), data->fd);

        return;
    }

    // Handle CLIENT_EVENT and CLIENT_CLOSED events.
    ctx->ev.events = EPOLLIN | EPOLLHUP;

    const AsyncData async_data = {.type = CLIENT_EVENT, .fd = client_fd};
    memcpy(&ctx->ev.data.u64, &async_data, sizeof(async_data));

    if ((ret = epoll_ctl(ctx->epoll_fd, EPOLL_CTL_ADD, client_fd, &ctx->ev)) < 0) {

        fprintf(stderr, "Failed to add client %d to epoll: %s\n", client_fd, strerror(ret));

        close(client_fd);
    }
}

void consume_request(AsyncCtx* ctx, const AsyncData* data) {
    const int32_t client_fd = data->fd;

    ProtocolRequest request;

    const int64_t bytes_received = recv(client_fd, &request, PROTOCOL_REQ_SIZE, 0);

    if (bytes_received != PROTOCOL_REQ_SIZE) {
        if (bytes_received == 0) {
            fprintf(stderr, "Received 0 bytes (perhaps closed)\n");
        } else if (bytes_received < 0) {
            fprintf(stderr, "Failed to receive request (perhaps closed): %s\n", strerror(errno));
        } else {
            fprintf(stderr, "Invalid request size: %lu\n", bytes_received);
        }

        return;
    }

    protocol_request_to_le(&request);

    JobData* task_data = scheduler_create_job_data(JOB_TYPE_FD, client_fd);
    scheduler_submit(ctx->scheduler, &request, task_data);
}

void remove_client(const AsyncCtx* ctx, const AsyncData* data) {
    epoll_ctl(ctx->epoll_fd, EPOLL_CTL_DEL, data->fd, NULL);
}
