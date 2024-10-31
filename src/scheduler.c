#include "scheduler.h"

void scheduler_park_thread(Scheduler* scheduler, const uint32_t version) {
    while (scheduler->running) {
        // Read version to check if we should continue.
        spin_rwlock_rdlock(&scheduler->swap_rwlock);
        const bool same_version = scheduler->sched_jobs_r->v == version;
        spin_rwlock_rdunlock(&scheduler->swap_rwlock);

        if (!same_version) {
            break;
        }

        struct timespec ts = {.tv_sec = 0, .tv_nsec = 100000};
        const int64_t s = futex(&scheduler->futex_waker, FUTEX_WAIT, 0, &ts, NULL, 0);

        if (s == -1) {
            if (errno == EAGAIN) {
                break;
            }

            if (errno == EINTR) {
                continue;
            }

            if (errno == ETIMEDOUT) {
                continue;
            }

            __builtin_unreachable();
        }

        break;
    }
}

void scheduler_wake_all_threads(Scheduler* scheduler) {
    const int64_t s = futex(&scheduler->futex_waker, FUTEX_WAKE, UINT32_MAX, NULL, NULL, 0);

    if (s == -1) {
        __builtin_unreachable();
    }
}

inline void scheduler_job_rc_enter(Scheduler* scheduler, const uint32_t job_idx) {
    spin_rwlock_rdlock(&scheduler->jobs_rwlock);
    __atomic_add_fetch(&(scheduler)->jobs.data[job_idx].rc, 1, __ATOMIC_RELAXED);
    spin_rwlock_rdunlock(&scheduler->jobs_rwlock);
}

inline void scheduler_job_rc_leave(Scheduler* scheduler, const uint32_t job_idx) {
    spin_rwlock_rdlock(&scheduler->jobs_rwlock);
    __atomic_sub_fetch(&(scheduler)->jobs.data[job_idx].rc, 1, __ATOMIC_RELAXED);
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

    scheduler->futex_waker = 0;
    scheduler->running = true;

    return scheduler;
}

void scheduler_destroy(Scheduler* scheduler) {
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

uint32_t scheduler_submit(Scheduler* scheduler, const struct ProtocolRequest* req, struct JobData* data) {
    // Process pending cache entries.
    cache_process_pending(scheduler->cache);

    const uint64_t* cached_answer;

    radix_tree_get(&scheduler->cache->tree, req->hash, SHA256_DIGEST_LENGTH, &cached_answer);

    if (cached_answer != NULL) {
        data->answer = *cached_answer;
        scheduler_job_notify(data);
        return UINT32_MAX;
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

    const bool will_grow = scheduler->jobs.free == 0;

    if (will_grow) {
        spin_rwlock_wrlock(&scheduler->jobs_rwlock);
    }

    const uint32_t idx = freelist_insert(&scheduler->jobs, job);

    if (will_grow) {
        spin_rwlock_wrunlock(&scheduler->jobs_rwlock);
    }

    // Ensure writer buffer is up-to-date.
    priority_heap_copy(&(scheduler->sched_jobs_w->p), &(scheduler->sched_jobs_r->p));
    scheduler->sched_jobs_w->v = scheduler->sched_jobs_r->v;

    // Purge all from priority heap where job is done.
    for (uint32_t i = 0; i < scheduler->sched_jobs_w->p.len; i++) {
        const struct Job* some_job = &scheduler->jobs.data[scheduler->sched_jobs_w->p.data[i].elem];

        // Only remove the job once we can remove it from the freelist!
        if (scheduler_job_is_done(some_job) && atomic_load(&job.rc)) {
            freelist_remove(&scheduler->jobs, scheduler->sched_jobs_w->p.data[i].elem);
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
    scheduler_wake_all_threads(scheduler);

    return job.id;
}

void scheduler_cancel(const Scheduler* scheduler, const uint32_t job_id) {
    for (uint32_t i = 0; i < scheduler->sched_jobs_r->p.len; i++) {
        struct Job* job = &scheduler->jobs.data[scheduler->sched_jobs_r->p.data[i].elem];

        if (job->id != job_id) {
            continue;
        }

        // Mark job as done.
        atomic_store(&job->block_idx, job->block_count);
    }
}

bool scheduler_schedule(
    Scheduler* scheduler, struct ScheduledJobs* local_sched_jobs, uint32_t* last_job_id, HashDigest target,
    uint32_t* job_idx, uint64_t* start, uint64_t* end
) {
    // Potentially update local copy of scheduled jobs.
    spin_rwlock_rdlock(&scheduler->swap_rwlock);

    if (scheduler->sched_jobs_r->v > local_sched_jobs->v) {
        priority_heap_copy(&local_sched_jobs->p, &scheduler->sched_jobs_r->p);
        local_sched_jobs->v = scheduler->sched_jobs_r->v;
    }

    spin_rwlock_rdunlock(&scheduler->swap_rwlock);

    // Find next job to work at.
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

    // Park thread until version is updated.
    // We check the version at a small interval to avoid
    // a race condition here.
    scheduler_park_thread(scheduler, local_sched_jobs->v);

    return false;
}

void scheduler_job_done(Scheduler* scheduler, const uint32_t job_idx, const uint64_t answer) {
    struct JobData* data = NULL;

    spin_rwlock_rdlock(&scheduler->jobs_rwlock);
    struct Job* job = &scheduler->jobs.data[job_idx];
    atomic_store(&job->block_idx, job->block_count);
    data = job->data;
    job->data = NULL;
    spin_rwlock_rdunlock(&scheduler->jobs_rwlock);

    if (data == NULL) {
        return;
    }

    data->answer = answer;

    // Notify client of response.
    scheduler_job_notify(data);
}

void scheduler_job_notify(struct JobData* data) {
    struct ProtocolResponse response = {.answer = data->answer};
    protocol_response_to_be(&response);

    switch (data->type) {

    case JOB_TYPE_FUTEX:
        if (__sync_bool_compare_and_swap(&data->futex, 0, 1)) {
            futex(&data->futex, FUTEX_WAKE, 1, NULL, NULL, 0);
        }
        break;
    case JOB_TYPE_FD:
        send(data->fd, &response, PROTOCOL_RES_SIZE, 0);
        close(data->fd);
        free(data);
        break;
    }
}

void scheduler_close(Scheduler* scheduler) {
    scheduler->running = false;
    scheduler_wake_all_threads(scheduler);
}
