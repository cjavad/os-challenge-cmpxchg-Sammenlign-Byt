#pragma once

#include "sha256/sha256.h"
#include "scheduler.h"
#include <pthread.h>

struct WorkerState {
    Scheduler* scheduler;
    pthread_t thread;
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

int cpu_core_count();