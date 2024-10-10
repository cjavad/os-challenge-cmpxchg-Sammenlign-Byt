#pragma once

#include "../hash.h"
#include "scheduler.h"
#include <pthread.h>

struct WorkerState {
    Scheduler* scheduler;
    pthread_t thread;
    bool running;
};

typedef struct WorkerState WorkerState;

struct WorkerPool {
    Scheduler* scheduler;
    WorkerState* workers;
    size_t size;
};

typedef struct WorkerPool WorkerPool;

WorkerPool* worker_create_pool(size_t size, Scheduler* scheduler);
void worker_destroy_pool(WorkerPool* pool);

void* worker_thread(void* arguments);

int worker_pool_submit(const WorkerPool* pool, Task* state);

int cpu_core_count();