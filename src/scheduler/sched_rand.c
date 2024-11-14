#include "sched_rand.h"

#include "../bits/minmax.h"
#include "../bits/prng.h"
#include "../cache.h"
#include "../bits/futex.h"
#include "../sha256/sha256.h"

struct RandScheduler* scheduler_rand_create(
    const uint32_t default_cap
) {
    struct RandScheduler* scheduler = calloc(1, sizeof(struct RandScheduler));

    scheduler->block_size = SCHEDULER_RAND_BLOCK_SIZE;

    scheduler->lock = (pthread_mutex_t)PTHREAD_MUTEX_INITIALIZER;
    ringbuffer_init(&scheduler->pending_jobs, default_cap);
    vec_init(&scheduler->jobs, default_cap);

    scheduler->priority_sum = 0;
    scheduler->task_idx = 0;
    scheduler->priority_idx = 0;
    scheduler->prng_state = 0xbeefbaad;

    scheduler_base_init(&scheduler->base, default_cap);

    return scheduler;
}

void scheduler_rand_destroy(
    struct RandScheduler* scheduler
) {
    pthread_mutex_destroy(&scheduler->lock);
    ringbuffer_destroy(&scheduler->pending_jobs);
    vec_destroy(&scheduler->jobs);
    scheduler_base_destroy(&scheduler->base);
    free(scheduler);
}

SchedulerJobId scheduler_rand_submit(
    struct RandScheduler* scheduler,
    const struct ProtocolRequest* request,
    struct SchedulerJobRecipient* recipient
) {
    cache_process_pending(scheduler->base.cache);

    uint64_t* cached_answer;

    radix_tree_get(
        &scheduler->base.cache->tree,
        request->hash,
        SHA256_DIGEST_LENGTH,
        &cached_answer
    );

    if (cached_answer != NULL) {
        scheduler_job_notify_recipient(recipient, *cached_answer);
        return SCHEDULER_NO_JOB_ID_SENTINEL;
    }

    const uint8_t priority = max(request->priority, 1);

    struct RandSchedulerJob job = {
        .start = request->start,
        .end = request->end,
        .priority = priority,
        .recipient = recipient,
        .id = scheduler->next_job_id++,
    };

    memcpy(job.hash, request->hash, SHA256_DIGEST_LENGTH);

    ringbuffer_push(&scheduler->pending_jobs, job);

    scheduler_wake_workers(&scheduler->base);

    return job.id;
}

// SAFETY: Requires ownership of lock
void scheduler_rand_push_job(
    struct RandScheduler* scheduler,
    const struct RandSchedulerJob* job
) {
    // It is important we increment the priority sum before the job_len.
    scheduler->priority_sum += job->priority;
    vec_push(&scheduler->jobs, *job);
}

// SAFETY: Requires ownership of lock
void scheduler_rand_remove_swap_job(
    struct RandScheduler* scheduler,
    const uint32_t idx
) {
    struct RandSchedulerJob* task = &scheduler->jobs.data[idx];
    const struct RandSchedulerJob* last =
        &scheduler->jobs.data[scheduler->jobs.len - 1];

    scheduler->priority_sum -= task->priority;
    scheduler->jobs.len -= 1;

    memcpy(task, last, sizeof(struct RandSchedulerJob));
}

void scheduler_rand_process_pending_jobs(
    struct RandScheduler* scheduler
) {
    const uint32_t head = ringbuffer_head(&scheduler->pending_jobs);
    uint32_t tail = ringbuffer_tail(&scheduler->pending_jobs);
    struct RandSchedulerJob pending_job;

    while (head != tail) {
        ringbuffer_pop(&scheduler->pending_jobs, &pending_job);
        tail = ringbuffer_tail(&scheduler->pending_jobs);

        scheduler_rand_push_job(scheduler, &pending_job);
    }
}

void scheduler_rand_cancel(
    struct RandScheduler* scheduler,
    const SchedulerJobId job_id
) {
    pthread_mutex_lock(&scheduler->lock);

    for (uint32_t i = 0; i < scheduler->jobs.len; i++) {
        const struct RandSchedulerJob* job = &scheduler->jobs.data[i];

        if (job->id == job_id) {
            scheduler_rand_remove_swap_job(scheduler, i);

            scheduler->task_idx = 0;
            scheduler->priority_idx = 0;
            break;
        }
    }

    pthread_mutex_unlock(&scheduler->lock);
}

bool scheduler_rand_schedule(
    struct RandScheduler* scheduler,
    struct RandSchedulerTask* task
) {
    while (true) {
        pthread_mutex_lock(&scheduler->lock);

        if (!scheduler->base.running) {
            pthread_mutex_unlock(&scheduler->lock);
            return false;
        }

        scheduler_rand_process_pending_jobs(scheduler);

        // Exit loop with lock
        if (scheduler->jobs.len > 0) {
            break;
        }

        // Give up lock and wait for next job
        pthread_mutex_unlock(&scheduler->lock);

        while (scheduler->base.running && scheduler->jobs.len == 0) {
            scheduler_yield_worker(&scheduler->base);
        }
    }

    uint32_t offset =
        xorshift32_prng_next(&scheduler->prng_state) % scheduler->priority_sum;

    for (;;) {
        const struct RandSchedulerJob* current =
            &scheduler->jobs.data[scheduler->task_idx];
        const uint32_t remaining = current->priority - scheduler->priority_idx;

        if (remaining < offset) {
            scheduler->task_idx =
                (scheduler->task_idx + 1) % scheduler->jobs.len;
            scheduler->priority_idx = 0;

            offset -= remaining;
            continue;
        }

        scheduler->priority_idx += offset;
        break;
    }

    struct RandSchedulerJob* current =
        &scheduler->jobs.data[scheduler->task_idx];

    task->id = current->id;
    task->start = current->start;
    task->end = current->start + scheduler->block_size;
    task->end = min(current->end, task->end);
    task->recipient = current->recipient;

    memcpy(task->hash, current->hash, sizeof(HashDigest));

    current->start = task->end;

    if (current->start == current->end) {
        scheduler_rand_remove_swap_job(scheduler, scheduler->task_idx);

        // reset the scheduler state, or segfaults be upon us
        scheduler->task_idx = 0;
        scheduler->priority_idx = 0;
    }

    pthread_mutex_unlock(&scheduler->lock);

    return true;
}

void* scheduler_rand_worker(
    struct RandScheduler* scheduler
) {

    while (scheduler->base.running) {
        struct RandSchedulerTask task;

        if (!scheduler_rand_schedule(scheduler, &task)) {
            continue;
        }

        const uint64_t answer =
            reverse_sha256_x4(task.start, task.end, task.hash);

        if (answer == SCHEDULER_NO_ANSWER_SENTINEL) {
            continue;
        }

        // Cancel current job if it is not done.
        scheduler_rand_cancel(scheduler, task.id);
        // Notify recipient of the answer.
        scheduler_job_notify_recipient(task.recipient, answer);
        // Insert answer into cache.
        cache_insert_pending(scheduler->base.cache, task.hash, answer);
    }

    return NULL;
}