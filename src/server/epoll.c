#include "epoll.h"
#include "../protocol.h"
#include "../scheduler/sched_priority.h"
#include "../worker.h"
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>

void accept_client(const struct EpollServerCtx* ctx, union EpollEventData* data);
void consume_request(const struct EpollServerCtx* ctx, const union EpollEventData* data);
void remove_client(const struct EpollServerCtx* ctx, const union EpollEventData* data);

int epoll_server_init(const Server* server, struct EpollServerCtx* ctx) {
    int ret = 0;
    memset(ctx, 0, sizeof(struct EpollServerCtx));

    ret = epoll_create(EPOLL_MAX_EVENTS);

    if (ret < 0) {
        printf("Failed to create epoll fd for server context: %s\n", strerror(errno));
        return ret;
    }

    // Setup server fd.
    ctx->epoll_fd = ret;

    struct epoll_event ev = {0};
    ev.events = EPOLLIN;

    const union EpollEventData async_data = {.type = SERVER_ACCEPT, .fd = server->fd};
    memcpy(&ev.data.u64, &async_data, sizeof(async_data));

    if ((ret = epoll_ctl(ctx->epoll_fd, EPOLL_CTL_ADD, server->fd, &ev)) < 0) {
        printf("Failed to add server fd to epoll fd for server context: %s\n", strerror(errno));
        return ret;
    }

    // Spawn worker pool
    ctx->scheduler = (void*)scheduler_priority_create(8);
    ctx->worker_pool =
        worker_create_pool(cpu_core_count(), ctx->scheduler, (void* (*)(void*))scheduler_priority_worker);

    return ret;
}

int epoll_server_poll(const struct EpollServerCtx* ctx) {
    struct epoll_event events[EPOLL_MAX_EVENTS];

    const int32_t new_events = epoll_wait(ctx->epoll_fd, events, EPOLL_MAX_EVENTS, -1);

    if (new_events < 0) {
        return new_events;
    }

    for (int i = 0; i < new_events; i++) {
        const struct epoll_event* event = &events[i];
        union EpollEventData data;
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

int epoll_server_exit(const Server* server, const struct EpollServerCtx* ctx) {
    // it's *very* important that the worker pool is destroyed before the scheduler
    // since the worker threads are still running and may access the scheduler.
    worker_destroy_pool(ctx->worker_pool);
    scheduler_priority_destroy((void*)ctx->scheduler);
    return close(ctx->epoll_fd) + close(server->fd);
}

void accept_client(const struct EpollServerCtx* ctx, union EpollEventData* data) {
    int ret = 0;

    struct sockaddr_in addr;
    uint32_t client_len = sizeof(addr);

    const int32_t client_fd = accept(data->fd, (struct sockaddr*)&addr, &client_len);

    if (client_fd < 0) {
        fprintf(stderr, "Failed to accept client: %s from %d\n", strerror(errno), data->fd);
        return;
    }

    if ((ret = fcntl(client_fd, F_SETFL, O_NONBLOCK) < 0)) {
        fprintf(stderr, "Failed to set client %d to non-blocking (%d): %s\n", client_fd, ret, strerror(errno));
        close(client_fd);
        return;
    }

    struct epoll_event ev = {0};

    // Handle CLIENT_EVENT and CLIENT_CLOSED events.
    ev.events = EPOLLIN | EPOLLHUP;

    const union EpollEventData async_data = {.type = CLIENT_EVENT, .fd = client_fd};
    memcpy(&ev.data.u64, &async_data, sizeof(async_data));

    if ((ret = epoll_ctl(ctx->epoll_fd, EPOLL_CTL_ADD, client_fd, &ev)) < 0) {
        fprintf(stderr, "Failed to add client %d to epoll: %d. errno: %s\n", client_fd, ret, strerror(errno));

        close(client_fd);
    }
}

void consume_request(const struct EpollServerCtx* ctx, const union EpollEventData* data) {
    const int32_t client_fd = data->fd;

    struct ProtocolRequest request;

    const int64_t bytes_received = recv(client_fd, &request, PROTOCOL_REQ_SIZE, 0);

    if (bytes_received != PROTOCOL_REQ_SIZE) {
        if (bytes_received == 0) {
            // fprintf(stderr, "Received 0 bytes (perhaps closed)\n");
        } else if (bytes_received < 0) {
            // fprintf(stderr, "Failed to receive request (perhaps closed): %s\n", strerror(errno));
        } else {
            fprintf(stderr, "Invalid request size: %lu\n", bytes_received);
        }

        return;
    }

    protocol_request_to_le(&request);

    struct SchedulerJobRecipient* recipient =
        scheduler_create_job_recipient(SCHEDULER_JOB_RECIPIENT_TYPE_FD, client_fd);

    scheduler_priority_submit((void*)ctx->scheduler, &request, recipient);
}

void remove_client(const struct EpollServerCtx* ctx, const union EpollEventData* data) {
    epoll_ctl(ctx->epoll_fd, EPOLL_CTL_DEL, data->fd, NULL);
}
