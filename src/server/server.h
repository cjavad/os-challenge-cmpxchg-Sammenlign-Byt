#pragma once

#include "../config.h"
#include "../scheduler/generic.h"
#include <netinet/in.h>

#ifndef WORKER_CONCURRENCY_MULTIPLIER
#define WORKER_CONCURRENCY_MULTIPLIER 1
#endif

struct Server {
    int fd;
    struct sockaddr_in addr;
};

struct ServerScheduler {
    struct WorkerPool* worker_pool;
    SchedulerImpl* scheduler;
};

// Main server loop
int server(uint16_t port);

// Setup socket fd (shared between all implementations)
int server_init(struct Server* server, int port);
int server_listen(struct Server* server, int backlog);
int server_close(const struct Server* server);

void server_scheduler_init(struct ServerScheduler* sched, uint32_t default_cap);
void server_scheduler_destroy(struct ServerScheduler* sched);
