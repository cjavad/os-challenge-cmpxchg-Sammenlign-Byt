#include "server.h"
#include "worker_pool.h"
#include <errno.h>
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