#pragma once

#include "scheduler/scheduler.h"
#include <pthread.h>

struct WorkerState {
    struct SchedulerBase* scheduler;
    pthread_t thread;
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

void* worker_thread(void* arguments);

int get_worker_count();