#include "server.h"
#include "generic.h"
#include "../config.h"
#include "worker_pool.h"
#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

int server_init(
    struct Server* server,
    const int port
) {
    server->fd = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);

    if (server->fd < 0) {
        fprintf(stderr, "Failed to create socket: %s\n", strerror(errno));
        return -1;
    }

    const int opt_level = 1;

    if (setsockopt(
            server->fd,
            SOL_SOCKET,
            SO_REUSEADDR,
            &opt_level,
            sizeof(opt_level)
        ) < 0) {
        fprintf(stderr, "Failed to set socket options: %s\n", strerror(errno));
        return -1;
    }

    server->addr.sin_family = AF_INET;
    server->addr.sin_port = htons(port);
    server->addr.sin_addr.s_addr = INADDR_ANY;

    return 0;
}

int server_listen(
    struct Server* server,
    const int backlog
) {
    if (bind(
            server->fd,
            (struct sockaddr*)&server->addr,
            sizeof(struct sockaddr)
        ) < 0) {
        fprintf(stderr, "Failed to bin server to port: %s\n", strerror(errno));
        return -1;
    }

    if (listen(server->fd, backlog) < 0) {
        fprintf(stderr,
                "Failed to have server listen on port: %s\n",
                strerror(errno));
        return -1;
    }

    return 0;
}

int server_close(
    const struct Server* server
) {
    return close(server->fd);
}

void server_scheduler_init(struct ServerScheduler* sched,
                           const uint32_t default_cap) {
    // Spawn worker pool
    sched->scheduler = scheduler_create(sched->scheduler, default_cap);
    sched->worker_pool = worker_create_pool(
        worker_pool_get_concurrency(),
        (void*)sched->scheduler,
        scheduler_worker_thread(sched->scheduler)
    );
}

void server_scheduler_destroy(struct ServerScheduler* sched) {
    // it's *very* important that the worker pool is destroyed before the
    // scheduler since the worker threads are still running and may access the
    // scheduler.
    worker_destroy_pool(sched->worker_pool);
    scheduler_destroy(sched->scheduler);
}

// Main function
static volatile sig_atomic_t stop_flag = 0;

static void signal_handler(
    const int signal
) {
    (void)signal;
    stop_flag = 1;
}

int server(
    const uint16_t port
) {
    // Setup signal handler
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    ServerImplCtx ctx = {0};

    if (server_init(&ctx.server, port) < 0) {
        fprintf(stderr, "Failed to initialize server: %s\n", strerror(errno));
        return 1;
    }

    if (server_listen(&ctx.server, 512) < 0) {
        fprintf(stderr,
                "Failed to listen on port %d: %s\n",
                port,
                strerror(errno));
        return 1;
    }

    fprintf(stderr, "Listening on port %i\n", port);

    if (server_impl_init(&ctx) < 0) {
        fprintf(stderr,
                "Failed to initialize async server: %s\n",
                strerror(errno));
        return 1;
    }

    while (!stop_flag) {
        if (server_impl_poll(&ctx) >= 0 || errno == EAGAIN) {
            continue;
        }

        fprintf(stderr, "Failed to poll server: %s\n", strerror(errno));
        break;
    }

    server_impl_exit(&ctx);

    fprintf(stderr, "Closed server\n");

    return 0;
}