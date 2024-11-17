#pragma once

#include "../config.h"
#include "../scheduler/scheduler.h"
#include <pthread.h>
#include <stdbool.h>

#ifndef WORKER_CONCURRENCY_EXTRA
#define WORKER_CONCURRENCY_EXTRA 0
#endif

#ifndef WORKER_CONCURRENCY_MULTIPLIER
#define WORKER_CONCURRENCY_MULTIPLIER 1
#endif

struct WorkerState {
    pthread_t thread;
    struct SchedulerBase* scheduler;
};

struct WorkerPool {
    struct SchedulerBase* scheduler;
    struct WorkerState* workers;
    size_t size;
};

struct WorkerPool* worker_create_pool(
    size_t size,
    struct SchedulerBase* scheduler,
    void* (*worker_thread)(void*)
);

void worker_destroy_pool(struct WorkerPool* pool);
int worker_pool_get_concurrency();
bool worker_set_nice(int nice);