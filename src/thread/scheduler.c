#include "scheduler.h"

Scheduler* scheduler_create(uint32_t cap) {
    Scheduler* scheduler = malloc(sizeof(Scheduler));

    scheduler->block_size = 1 << 10;

    pthread_mutex_init(&scheduler->mutex, NULL);

    scheduler->jobs = malloc(sizeof(Task) * cap);
    scheduler->job_cap = cap;
    scheduler->job_len = 0;

    scheduler->priority_sum = 0;

    scheduler->task_idx = 0;
    scheduler->priority_idx = 0;

    scheduler->prng_state = 0xbeefbaad;

    return scheduler;
}

void scheduler_destroy(Scheduler* scheduler) {
    pthread_mutex_destroy(&scheduler->mutex);
    free(scheduler->jobs);
    free(scheduler);
}

static void scheduler_grow_jobs(Scheduler* scheduler) {
    uint32_t new_cap = scheduler->job_cap * 2;

    scheduler->jobs = realloc(scheduler->jobs, sizeof(Task) * new_cap);
    scheduler->job_cap = new_cap;
}

static void scheduler_push_job(Scheduler* scheduler, const ProtocolRequest* req, uint64_t id, JobData data) {
    if (scheduler->job_len >= scheduler->job_cap) {
        scheduler_grow_jobs(scheduler);
    }

    Job* job = &scheduler->jobs[scheduler->job_len];
    memcpy(&job->hash, req->hash, sizeof(HashDigest));
    job->start = req->start;
    job->end = req->end;
    job->priority = req->priority;
    job->id = id;
    job->data = data;

    scheduler->job_len += 1;
}

static void scheduler_remove_swap_job(Scheduler* scheduler, uint32_t idx) {
    Job* task = &scheduler->jobs[idx];
    Job* last = &scheduler->jobs[scheduler->job_len - 1];

    memcpy(task, last, sizeof(Job));
    scheduler->job_len -= 1;
}

uint64_t scheduler_submit(Scheduler* scheduler, const ProtocolRequest* req, JobData data) {
    pthread_mutex_lock(&scheduler->mutex);

    uint64_t job_id = scheduler->next_job_id++;

    scheduler_push_job(scheduler, req, job_id, data);
    scheduler->priority_sum += req->priority;

    pthread_mutex_unlock(&scheduler->mutex);

    return job_id;
}

void scheduler_terminate(Scheduler* scheduler, uint64_t job_id) {
    pthread_mutex_lock(&scheduler->mutex);

    for (uint32_t i = 0; i < scheduler->job_len; i++) {
        Job* job = &scheduler->jobs[i];

        if (job->id == job_id) {
            scheduler_remove_swap_job(scheduler, i);
            scheduler->priority_sum -= job->priority;

            scheduler->task_idx = 0;
            scheduler->priority_idx = 0;
            break;
        }
    }

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

    if (scheduler->job_len == 0) {
        pthread_mutex_unlock(&scheduler->mutex);
        return false;
    }

    uint32_t offset = prng_next(&scheduler->prng_state) % scheduler->priority_sum;

    for (;;) {
        Job* current = &scheduler->jobs[scheduler->task_idx];
        uint32_t remaining = current->priority - scheduler->priority_idx;

        if (remaining < offset) {
            scheduler->task_idx += 1;
            scheduler->task_idx %= scheduler->job_len;
            scheduler->priority_idx = 0;

            offset -= remaining;
            continue;
        }

        scheduler->priority_idx += offset;
        break;
    }

    Job* current = &scheduler->jobs[scheduler->task_idx];

    task->start = current->start;
    task->end = current->start + scheduler->block_size;
    task->end = MIN(current->end, task->end);

    memcpy(task->hash, current->hash, sizeof(HashDigest));
    task->data = current->data;

    current->start = task->end;

    if (current->start == current->end) {
        scheduler_remove_swap_job(scheduler, scheduler->task_idx);
        scheduler->priority_sum -= current->priority;

        // reset the scheduler state, or segfaults be upon us
        scheduler->task_idx = 0;
        scheduler->priority_idx = 0;
    }

    pthread_mutex_unlock(&scheduler->mutex);

    return true;
}

void scheduler_empty(Scheduler* scheduler) {
    pthread_mutex_lock(&scheduler->mutex);

    scheduler->job_len = 0;
    scheduler->priority_sum = 0;

    pthread_mutex_unlock(&scheduler->mutex);
}
