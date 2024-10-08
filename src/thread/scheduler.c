#include "scheduler.h"

Scheduler* scheduler_create(uint32_t cap) {
    Scheduler* scheduler = malloc(sizeof(Scheduler));

    scheduler->block_size = 1 << 10;

    pthread_mutex_init(&scheduler->mutex, NULL);

    scheduler->tasks = malloc(sizeof(Task) * cap);
    scheduler->task_cap = cap;
    scheduler->task_len = 0;

    scheduler->priority_sum = 0;

    scheduler->task_idx = 0;
    scheduler->priority_idx = 0;

    scheduler->prng_state = 0xbeefbaad;

    return scheduler;
}

void scheduler_destroy(Scheduler* scheduler) {
    pthread_mutex_destroy(&scheduler->mutex);
    free(scheduler->tasks);
    free(scheduler);
}

static void scheduler_grow(Scheduler* scheduler) {
    uint32_t new_cap = scheduler->task_cap * 2;

    scheduler->tasks = realloc(scheduler->tasks, sizeof(Task) * new_cap);
    scheduler->task_cap = new_cap;
}

static void scheduler_push_task(Scheduler* scheduler, const ProtocolRequest* req, TaskData data) {
    if (scheduler->task_len >= scheduler->task_cap) {
        scheduler_grow(scheduler);
    }

    TaskState* task = &scheduler->tasks[scheduler->task_len];
    memcpy(&task->hash, req->hash, sizeof(HashDigest));
    task->start = req->start;
    task->end = req->end;
    task->priority = req->priority;
    task->data = data;

    scheduler->task_len += 1;
}

static void scheduler_remove_swap(Scheduler* scheduler, uint32_t idx) {
    TaskState* task = &scheduler->tasks[idx];
    TaskState* last = &scheduler->tasks[scheduler->task_len - 1];

    memcpy(task, last, sizeof(TaskState));
    scheduler->task_len -= 1;
}

void scheduler_submit(Scheduler* scheduler, const ProtocolRequest* req, TaskData data) {
    pthread_mutex_lock(&scheduler->mutex);

    scheduler_push_task(scheduler, req, data);
    scheduler->priority_sum += req->priority;

    pthread_mutex_unlock(&scheduler->mutex);
}

// XORShift32 prng
//
// https://www.jstatsoft.org/article/view/v008i14
// https://en.wikipedia.org/wiki/Xorshift
static uint32_t prng_next(uint32_t* state) {
    *state ^= *state << 13;
    *state ^= *state >> 17;
    *state ^= *state << 5;

    return *state;
}

#define MIN(a, b) ((a) < (b) ? (a) : (b))

bool scheduler_schedule(Scheduler* scheduler, Task* task) {
    pthread_mutex_lock(&scheduler->mutex);

    if (scheduler->task_len == 0) {
        pthread_mutex_unlock(&scheduler->mutex);
        return false;
    }

    uint32_t offset = prng_next(&scheduler->prng_state) % scheduler->priority_sum;

    for (;;) {
        TaskState* current = &scheduler->tasks[scheduler->task_idx];
        uint32_t remaining = current->priority - scheduler->priority_idx;

        if (remaining < offset) {
            scheduler->task_idx += 1;
            scheduler->task_idx %= scheduler->task_len;
            scheduler->priority_idx = 0;

            offset -= remaining;
            continue;
        }

        scheduler->priority_idx += offset;
        break;
    }

    TaskState* current = &scheduler->tasks[scheduler->task_idx];

    task->start = current->start;
    task->end = current->start + scheduler->block_size;
    task->end = MIN(current->end, task->end);

    memcpy(task->hash, current->hash, sizeof(HashDigest));
    task->data = current->data;

    current->start = task->end;

    if (current->start == current->end) {
        scheduler_remove_swap(scheduler, scheduler->task_idx);
        scheduler->priority_sum -= current->priority;
    }

    pthread_mutex_unlock(&scheduler->mutex);

    return true;
}

void scheduler_empty(Scheduler* scheduler) {
    pthread_mutex_lock(&scheduler->mutex);

    scheduler->task_len = 0;
    scheduler->priority_sum = 0;

    pthread_mutex_unlock(&scheduler->mutex);
}
