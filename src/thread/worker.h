#pragma once

#include "queue.h"
#include "scheduler.h"
#include <pthread.h>
#include <sys/socket.h>

struct WorkerState {
    Queue* pending;
    pthread_t thread;
};

typedef struct WorkerState WorkerState;

struct WorkerPool {
    Queue* pending;
    WorkerState* workers;
    size_t size;
};

typedef struct WorkerPool WorkerPool;

WorkerPool* worker_create_pool(size_t size);
void worker_destroy_pool(WorkerPool* pool);

void* worker_thread(void* arguments);

int worker_pool_submit(const WorkerPool* pool, TaskState* state);
