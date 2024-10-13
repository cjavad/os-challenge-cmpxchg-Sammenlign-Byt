#include "scheduler.h"

#include "futex.h"

#include <assert.h>

#define SCHEDULER_BLOCK_SIZE (1 << 16)

Scheduler* scheduler_create(const uint32_t cap) {
    Scheduler* scheduler = calloc(1, sizeof(Scheduler));

    scheduler->block_size = SCHEDULER_BLOCK_SIZE;
    scheduler->running = true;

    scheduler->waker = (pthread_cond_t)PTHREAD_COND_INITIALIZER;
    scheduler->lock = (pthread_mutex_t)PTHREAD_MUTEX_INITIALIZER;

    scheduler->cache = cache_create(1024);

    ringbuffer_init(&scheduler->pending_jobs, 1024);
    vec_init(&scheduler->jobs, cap);

    scheduler->priority_sum = 0;

    scheduler->task_idx = 0;
    scheduler->priority_idx = 0;

    scheduler->prng_state = 0xbeefbaad;

    return scheduler;
}

void scheduler_destroy(Scheduler* scheduler) {
    pthread_cond_destroy(&scheduler->waker);
    pthread_mutex_destroy(&scheduler->lock);

    ringbuffer_destroy(&scheduler->pending_jobs);
    vec_destroy(&scheduler->jobs);

    free(scheduler);
}

JobData* scheduler_create_job_data(const JobType type, const uint32_t data) {
    JobData* job_data = calloc(1, sizeof(JobData));
    job_data->type = type;
    job_data->data = data;
    return job_data;
}
void scheduler_destroy_job_data(JobData* data) { free(data); }

uint64_t scheduler_submit(Scheduler* scheduler, ProtocolRequest* req, JobData* data) {
    // Process pending cache entries.
    cache_process_pending(scheduler->cache);

    const uint64_t cached_answer = cache_get(scheduler->cache, req->hash);

    if (cached_answer != 0) {
        scheduler_job_notify(data, &(ProtocolResponse){.answer = cached_answer});
        return 0;
    }

#ifdef DEBUG
    // protocol_debug_print_request(req);
#endif

    // If the priority is 0, set it to 1
    if (req->priority == 0) {
        req->priority = 1;
    }

    const Job job = {
        .start = req->start, .end = req->end, .priority = req->priority, .id = scheduler->next_job_id++, .data = data
    };

    memcpy(job.hash, req->hash, sizeof(HashDigest));

    ringbuffer_push(&scheduler->pending_jobs, job);

    // Wake single thread per new submission.
    pthread_cond_broadcast(&scheduler->waker);

    return job.id;
}

bool scheduler_terminate(Scheduler* scheduler, const uint64_t job_id) {
    bool found = false;

    pthread_mutex_lock(&scheduler->lock);

    for (uint32_t i = 0; i < scheduler->jobs.len; i++) {
        const Job* job = &scheduler->jobs.data[i];

        if (job->id == job_id) {
            scheduler_remove_swap_job(scheduler, i);

            scheduler->task_idx = 0;
            scheduler->priority_idx = 0;
            found = true;
            break;
        }
    }

    pthread_mutex_unlock(&scheduler->lock);

    return found;
}

void scheduler_job_done(const Scheduler* scheduler, const Task* task, ProtocolResponse* response) {
    // Notify client of response.
    scheduler_job_notify(task->data, response);

    // Store cache entry.
    // TODO: Fix temp workaround where we switch endianness back.
    cache_insert_pending(scheduler->cache, task->hash, __builtin_bswap64(response->answer));
}

void scheduler_job_notify(JobData* data, ProtocolResponse* response) {
    /// We assume this can only be called once.
    data->response = *response;

    switch (data->type) {
    case JOB_TYPE_FD:
        protocol_response_to_be(response);
        send(data->fd, response, PROTOCOL_RES_SIZE, 0);
        close(data->fd);
        break;

    case JOB_TYPE_FUTEX:
        futex_post(&data->futex);
        break;
    }

    // Cleanup job data once it is done.
    scheduler_destroy_job_data(data);
}

#define MIN(a, b) ((a) < (b) ? (a) : (b))

bool scheduler_schedule(Scheduler* scheduler, Task* task) {
    pthread_mutex_lock(&scheduler->lock);

    while (1) {
        // Check if we are still running.
        if (!scheduler->running) {
            pthread_mutex_unlock(&scheduler->lock);
            return false;
        }

        // Process pending jobs.
        scheduler_process_pending_jobs(scheduler);

        // When new jobs are submitted we check if any are pending.
        if (scheduler->jobs.len > 0) {
            break;
        }

        // Give up the lock to allow new jobs to come in.
        pthread_cond_wait(&scheduler->waker, &scheduler->lock);
    }

    assert(scheduler->jobs.len > 0);
    assert(scheduler->priority_sum > 0);

    uint32_t offset = xorshift32_prng_next(&scheduler->prng_state) % scheduler->priority_sum;

    for (;;) {
        const Job* current = &scheduler->jobs.data[scheduler->task_idx];
        const uint32_t remaining = current->priority - scheduler->priority_idx;

        if (remaining < offset) {
            scheduler->task_idx = (scheduler->task_idx + 1) % scheduler->jobs.len;
            scheduler->priority_idx = 0;

            offset -= remaining;
            continue;
        }

        scheduler->priority_idx += offset;
        break;
    }

    Job* current = &scheduler->jobs.data[scheduler->task_idx];

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

    pthread_mutex_unlock(&scheduler->lock);

    return true;
}

void scheduler_close(Scheduler* scheduler) {
    pthread_mutex_lock(&scheduler->lock);

    scheduler->running = false;
    scheduler->jobs.len = 0;
    scheduler->priority_sum = 0;

    pthread_mutex_unlock(&scheduler->lock);

    pthread_cond_broadcast(&scheduler->waker);
}

/// SAFETY: Requires ownership of lock
void scheduler_process_pending_jobs(Scheduler* scheduler) {
    const uint32_t head = ringbuffer_head(&scheduler->pending_jobs);
    uint32_t tail = ringbuffer_tail(&scheduler->pending_jobs);
    Job pending_job;

    while (head != tail) {
        ringbuffer_pop(&scheduler->pending_jobs, &pending_job);
        tail = ringbuffer_tail(&scheduler->pending_jobs);

        scheduler_push_job(scheduler, &pending_job);
    }
}

// SAFETY: Requires ownership of lock
void scheduler_push_job(Scheduler* scheduler, const Job* job) {
    // It is important we increment the priority sum before the job_len.
    scheduler->priority_sum += job->priority;
    vec_push(&scheduler->jobs, *job);
}

// SAFETY: Requires ownership of lock
void scheduler_remove_swap_job(Scheduler* scheduler, const uint32_t idx) {
    Job* task = &scheduler->jobs.data[idx];
    const Job* last = &scheduler->jobs.data[scheduler->jobs.len - 1];

    scheduler->priority_sum -= task->priority;
    scheduler->jobs.len -= 1;

    memcpy(task, last, sizeof(Job));
}

void scheduler_debug_print(const Scheduler* scheduler) {
    printf("Scheduler: job_len=%u job_cap=%u\n", scheduler->jobs.len, scheduler->jobs.cap);
    printf("           task_idx=%u priority_idx=%u\n", scheduler->task_idx, scheduler->priority_idx);

    for (uint32_t i = 0; i < scheduler->jobs.len; i++) {
        const Job* job = &scheduler->jobs.data[i];
        scheduler_debug_print_job(job);
    }
}

void scheduler_debug_print_job(const Job* job) {
    printf("Job: id=%lu start=%lu end=%lu priority=%u\n", job->id, job->start, job->end, job->priority);
}
