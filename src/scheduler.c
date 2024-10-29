#include "scheduler.h"

void scheduler_job_rc_enter(Scheduler* scheduler, const uint32_t job_idx) {
    spin_rwlock_rdlock(&scheduler->jobs_rwlock);
    __atomic_add_fetch(&(scheduler)->jobs.data[job_idx].rc, 1, __ATOMIC_RELAXED);
    spin_rwlock_rdunlock(&scheduler->jobs_rwlock);
}
void scheduler_job_rc_leave(Scheduler* scheduler, const uint32_t job_idx) {
    spin_rwlock_rdlock(&scheduler->jobs_rwlock);

    struct Job* job = &scheduler->jobs.data[job_idx];
    const uint32_t rc = __atomic_sub_fetch(&job->rc, 1, __ATOMIC_RELAXED);

    if (scheduler_job_is_done(job) && rc == 0) {
        // Thread safe remove.
        scheduler->jobs.indicices[__atomic_fetch_add(&scheduler->jobs.free, 1, __ATOMIC_RELAXED)] = job_idx;
    }

    spin_rwlock_rdunlock(&scheduler->jobs_rwlock);
}
Scheduler* scheduler_create(const uint32_t cap) {
    Scheduler* scheduler = calloc(1, sizeof(Scheduler));

    scheduler->cache = cache_create(cap);

    scheduler->sched_jobs_r = calloc(1, sizeof(struct ScheduledJobs));
    scheduler->sched_jobs_w = calloc(1, sizeof(struct ScheduledJobs));

    priority_heap_init(&scheduler->sched_jobs_r->p, cap);
    priority_heap_init(&scheduler->sched_jobs_w->p, cap);

    spin_rwlock_init(&scheduler->jobs_rwlock);
    spin_rwlock_init(&scheduler->swap_rwlock);

    freelist_init(&scheduler->jobs, cap);

    scheduler->running = true;

    scheduler->waker = (pthread_cond_t)PTHREAD_COND_INITIALIZER;
    scheduler->mutex = (pthread_mutex_t)PTHREAD_MUTEX_INITIALIZER;

    return scheduler;
}

void scheduler_destroy(Scheduler* scheduler) {
    pthread_mutex_destroy(&scheduler->mutex);
    pthread_cond_destroy(&scheduler->waker);
    cache_destroy(scheduler->cache);
    freelist_destroy(&scheduler->jobs);
    priority_heap_destroy(&scheduler->sched_jobs_r->p);
    priority_heap_destroy(&scheduler->sched_jobs_w->p);
    free(scheduler->sched_jobs_r);
    free(scheduler->sched_jobs_w);
    free(scheduler);
}

struct JobData* scheduler_create_job_data(const enum JobType type, const uint32_t data) {
    struct JobData* job_data = calloc(1, sizeof(struct JobData));
    job_data->type = type;
    job_data->data = data;
    return job_data;
}

void scheduler_submit(Scheduler* scheduler, const struct ProtocolRequest* req, struct JobData* data) {
    // Process pending cache entries.
    cache_process_pending(scheduler->cache);

    const uint64_t* cached_answer;

    radix_tree_get(&scheduler->cache->tree, req->hash, SHA256_DIGEST_LENGTH, &cached_answer);

    if (cached_answer != NULL) {
        data->answer = *cached_answer;
        scheduler_job_notify(data);
        return;
    }

#ifdef DEBUG
    // protocol_debug_print_request(req);
#endif
    const uint64_t difficulty = req->end - req->start;

    const uint64_t block_size = 1024;

    struct Job job = {
        .data = data,
        .block_size = block_size,
        .block_count = (difficulty + (block_size - 1)) / block_size,
        .id = scheduler->job_id++,
    };

    memcpy(&job.req, req, sizeof(struct ProtocolRequest));

    uint32_t idx;

    // Insert job into freelist.
    if (scheduler->jobs.free == 0) {
        spin_rwlock_wrlock(&scheduler->jobs_rwlock);
        idx = freelist_insert(&scheduler->jobs, job);
        spin_rwlock_wrunlock(&scheduler->jobs_rwlock);
    } else {
        idx = freelist_insert(&scheduler->jobs, job);
    }

    // Ensure writer buffer is up-to-date.
    priority_heap_copy(&(scheduler->sched_jobs_w->p), &(scheduler->sched_jobs_r->p));
    scheduler->sched_jobs_w->v = scheduler->sched_jobs_r->v;

    // Purge all from priority heap where job is done.
    for (uint32_t i = 0; i < scheduler->sched_jobs_w->p.len; i++) {
        const struct Job* some_job = &scheduler->jobs.data[scheduler->sched_jobs_w->p.data[i].elem];

        if (scheduler_job_is_done(some_job)) {
            priority_heap_remove(&scheduler->sched_jobs_w->p, i);
            i--;
        }
    }

    // Update writer buffer with the newest job submission.
    priority_heap_insert(&scheduler->sched_jobs_w->p, &idx, req->priority);

    // Increment version.
    scheduler->sched_jobs_w->v++;

    // Perform swap under lock.
    spin_rwlock_wrlock(&scheduler->swap_rwlock);
    struct ScheduledJobs* tmp = scheduler->sched_jobs_r;
    scheduler->sched_jobs_r = scheduler->sched_jobs_w;
    scheduler->sched_jobs_w = tmp;
    spin_rwlock_wrunlock(&scheduler->swap_rwlock);

    // Wake up workers.
    pthread_cond_broadcast(&scheduler->waker);
}

bool scheduler_schedule(
    Scheduler* scheduler, struct ScheduledJobs* local_sched_jobs, uint32_t* last_job_id, HashDigest target,
    uint32_t* job_idx, uint64_t* start, uint64_t* end
) {
    spin_rwlock_rdlock(&scheduler->swap_rwlock);

    if (scheduler->sched_jobs_r->v > local_sched_jobs->v) {
        priority_heap_copy(&local_sched_jobs->p, &scheduler->sched_jobs_r->p);
        local_sched_jobs->v = scheduler->sched_jobs_r->v;
    }

    spin_rwlock_rdunlock(&scheduler->swap_rwlock);

    *job_idx = UINT32_MAX;

    while (1) {
        const PriorityHeapNode(uint32_t) * node;
        priority_heap_get_max(&local_sched_jobs->p, (void*)&node);

        if (node == NULL) {
            break;
        }

        *job_idx = node->elem;

        spin_rwlock_rdlock(&scheduler->jobs_rwlock);

        struct Job* job = &scheduler->jobs.data[*job_idx];

        if (!scheduler_job_is_done(job)) {
            const uint64_t block_idx = atomic_fetch_add(&job->block_idx, 1);
            *start = job->req.start + block_idx * job->block_size;
            *end = min(job->req.end, *start + job->block_size);

            // Keep target up to date.
            if (*last_job_id != job->id) {
                memcpy(target, job->req.hash, SHA256_DIGEST_LENGTH);
                *last_job_id = job->id;
            }

            spin_rwlock_rdunlock(&scheduler->jobs_rwlock);

            return true;
        }

        spin_rwlock_rdunlock(&scheduler->jobs_rwlock);

        priority_heap_extract_max(&local_sched_jobs->p, NULL);
    }

    // Wait for new jobs.
    pthread_mutex_lock(&scheduler->mutex);
    pthread_cond_wait(&scheduler->waker, &scheduler->mutex);
    pthread_mutex_unlock(&scheduler->mutex);

    return false;
}

void scheduler_job_done(Scheduler* scheduler, const uint32_t job_idx, const uint64_t answer) {
    struct JobData* data = NULL;

    spin_rwlock_rdlock(&scheduler->jobs_rwlock);
    struct Job* job = &scheduler->jobs.data[job_idx];
    job->block_idx = job->block_count;
    data = job->data;
    spin_rwlock_rdunlock(&scheduler->jobs_rwlock);

    data->answer = answer;

    // Notify client of response.
    scheduler_job_notify(data);
}

void scheduler_job_notify(struct JobData* data) {
    struct ProtocolResponse response = {.answer = data->answer};
    protocol_response_to_be(&response);

    switch (data->type) {

    case JOB_TYPE_FUTEX:
        futex_post(&data->futex);
        break;
    case JOB_TYPE_FD:
        send(data->fd, &response, PROTOCOL_RES_SIZE, 0);
        close(data->fd);
        break;
    }
}

void scheduler_close(Scheduler* scheduler) { scheduler->running = false; }
