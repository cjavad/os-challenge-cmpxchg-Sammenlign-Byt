#pragma once

#include <netinet/in.h>
#include "../scheduler/generic.h"
#include "../config.h"

struct Server
{
    int fd;
    struct sockaddr_in addr;
};

struct ServerScheduler
{
    struct WorkerPool* worker_pool;
    SchedulerImpl* scheduler;
};

// Setup socket fd (shared between all implementations)
int server_init(struct Server* server, int port);
int server_listen(struct Server* server, int backlog);
int server_close(const struct Server* server);

void server_scheduler_init(struct ServerScheduler* sched, uint32_t default_cap);
void server_scheduler_destroy(struct ServerScheduler* sched);
