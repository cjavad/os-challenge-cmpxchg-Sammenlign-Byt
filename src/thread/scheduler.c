#include "scheduler.h"

#include "futex.h"

#include <assert.h>

#define SCHEDULER_BLOCK_SIZE (30000000 / 16)

void scheduler_grow_jobs(Scheduler* scheduler);
void scheduler_push_job(Scheduler* scheduler, const ProtocolRequest* req, uint64_t id, JobData* data);
void scheduler_remove_swap_job(Scheduler* scheduler, uint32_t idx);

Scheduler* scheduler_create(const uint32_t cap) {
    Scheduler* scheduler = calloc(1, sizeof(Scheduler));

    scheduler->block_size = SCHEDULER_BLOCK_SIZE;
    scheduler->running = true;

    scheduler->waker = (pthread_cond_t)PTHREAD_COND_INITIALIZER;
    scheduler->r_mutex = (pthread_mutex_t)PTHREAD_MUTEX_INITIALIZER;
    scheduler->w_mutex = (pthread_mutex_t)PTHREAD_MUTEX_INITIALIZER;

    scheduler->jobs = calloc(cap, sizeof(Job));
    scheduler->job_cap = cap;
    scheduler->job_len = 0;

    scheduler->priority_sum = 0;

    scheduler->task_idx = 0;
    scheduler->priority_idx = 0;

    scheduler->prng_state = 0xbeefbaad;

    return scheduler;
}

void scheduler_destroy(Scheduler* scheduler) {
    pthread_cond_destroy(&scheduler->waker);
    pthread_mutex_destroy(&scheduler->r_mutex);
    pthread_mutex_destroy(&scheduler->r_mutex);
    free(scheduler->jobs);
    free(scheduler);
}

JobData* scheduler_create_job_data(const JobType type, const uint32_t data) {
    JobData* job_data = calloc(1, sizeof(JobData));
    job_data->type = type;
    job_data->data = data;
    return job_data;
}

uint64_t scheduler_submit(Scheduler* scheduler, ProtocolRequest* req, JobData* data) {
    pthread_mutex_lock(&scheduler->w_mutex);

    // If the priority is 0, set it to 1
    if (req->priority == 0) {
        req->priority = 1;
    }

    const uint64_t job_id = scheduler->next_job_id++;

    scheduler_push_job(scheduler, req, job_id, data);

    pthread_mutex_unlock(&scheduler->w_mutex);

    // Wake single thread per new submission.
    pthread_cond_broadcast(&scheduler->waker);

    return job_id;
}

bool scheduler_terminate(Scheduler* scheduler, const uint64_t job_id) {
    bool found = false;

    pthread_mutex_lock(&scheduler->w_mutex);

    for (uint32_t i = 0; i < scheduler->job_len; i++) {
        const Job* job = &scheduler->jobs[i];

        if (job->id == job_id) {
            scheduler_remove_swap_job(scheduler, i);

            scheduler->task_idx = 0;
            scheduler->priority_idx = 0;
            found = true;
            break;
        }
    }

    pthread_mutex_unlock(&scheduler->w_mutex);

    return found;
}

void scheduler_job_done(Scheduler* scheduler, const Task* task, ProtocolResponse* response) {
    /// We assume this can only be called once.
    task->data->response = *response;

    switch (task->data->type) {
    case JOB_TYPE_FD:
        protocol_response_to_be(response);
        send(task->data->fd, response, PROTOCOL_RES_SIZE, 0);
        close(task->data->fd);
        break;

    case JOB_TYPE_FUTEX:
        futex_post(&task->data->futex);
        break;
    }
}

#define MIN(a, b) ((a) < (b) ? (a) : (b))

bool scheduler_schedule(Scheduler* scheduler, Task* task) {
    pthread_mutex_lock(&scheduler->r_mutex);
    pthread_mutex_lock(&scheduler->w_mutex);

    while (scheduler->job_len == 0) {
        // Give up the w_mutex to allow new jobs to come in.
        pthread_mutex_unlock(&scheduler->w_mutex);

        // Check if we are still running.
        if (!scheduler->running) {
            pthread_mutex_unlock(&scheduler->r_mutex);
            return false;
        }

        // Wait for new jobs to come in.
        pthread_cond_wait(&scheduler->waker, &scheduler->r_mutex);

        // Attempt to reclaim w_mutex, this can only race with job submission
        // not other worker threads as they are still waiting on r_mutex.
        pthread_mutex_lock(&scheduler->w_mutex);
    }

    // Give up the r_mutex so another waiter can queue up.
    pthread_mutex_unlock(&scheduler->r_mutex);

    assert(scheduler->job_len > 0);
    assert(scheduler->priority_sum > 0);

    uint32_t offset = xorshift32_prng_next(&scheduler->prng_state) % scheduler->priority_sum;

    for (;;) {
        const Job* current = &scheduler->jobs[scheduler->task_idx];
        const uint32_t remaining = current->priority - scheduler->priority_idx;

        if (remaining < offset) {
            scheduler->task_idx = (scheduler->task_idx + 1) % scheduler->job_len;
            scheduler->priority_idx = 0;

            offset -= remaining;
            continue;
        }

        scheduler->priority_idx += offset;
        break;
    }

    Job* current = &scheduler->jobs[scheduler->task_idx];

    task->job_id = current->id;
    task->start = current->start;
    task->end = current->start + scheduler->block_size;
    task->end = MIN(current->end, task->end);

    memcpy(task->hash, current->hash, sizeof(HashDigest));
    task->data = current->data;

    current->start = task->end;

    if (current->start == current->end) {
        scheduler_remove_swap_job(scheduler, scheduler->task_idx);

        // reset the scheduler state, or segfaults be upon us
        scheduler->task_idx = 0;
        scheduler->priority_idx = 0;
    }

    pthread_mutex_unlock(&scheduler->w_mutex);

    return true;
}

void scheduler_close(Scheduler* scheduler) {
    pthread_mutex_lock(&scheduler->w_mutex);

    scheduler->running = false;
    scheduler->job_len = 0;
    scheduler->priority_sum = 0;

    pthread_mutex_unlock(&scheduler->w_mutex);

    pthread_cond_broadcast(&scheduler->waker);
}

void scheduler_debug_print(const Scheduler* scheduler) {
    printf("Scheduler: job_len=%u job_cap=%u\n", scheduler->job_len, scheduler->job_cap);
    printf("           task_idx=%u priority_idx=%u\n", scheduler->task_idx, scheduler->priority_idx);

    for (uint32_t i = 0; i < scheduler->job_len; i++) {
        const Job* job = &scheduler->jobs[i];
        scheduler_debug_print_job(job);
    }
}

void scheduler_debug_print_job(const Job* job) {
    printf("Job: id=%lu start=%lu end=%lu priority=%u\n", job->id, job->start, job->end, job->priority);
}

// SAFETY: Requires ownership of w_mutex
void scheduler_grow_jobs(Scheduler* scheduler) {
    const uint32_t new_cap = scheduler->job_cap * 2;
    Job* new_jobs = reallocarray(scheduler->jobs, new_cap, sizeof(Job));

    if (new_jobs == NULL) {
        fprintf(stderr, "Failed to grow jobs array - segfault imminent\n");
        return;
    }

    scheduler->jobs = new_jobs;
    scheduler->job_cap = new_cap;
}

// SAFETY: Requires ownership of w_mutex
void scheduler_push_job(Scheduler* scheduler, const ProtocolRequest* req, const uint64_t id, JobData* data) {
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

    // It is important we increment the priority sum before the job_len.
    scheduler->priority_sum += req->priority;
    scheduler->job_len += 1;
}

// SAFETY: Requires ownership of w_mutex
void scheduler_remove_swap_job(Scheduler* scheduler, const uint32_t idx) {
    Job* task = &scheduler->jobs[idx];
    const Job* last = &scheduler->jobs[scheduler->job_len - 1];

    scheduler->priority_sum -= task->priority;
    scheduler->job_len -= 1;

    memcpy(task, last, sizeof(Job));
}
