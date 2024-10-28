#include "scheduler.h"

Scheduler* scheduler_create(const uint32_t cap) {
    Scheduler* scheduler = calloc(1, sizeof(Scheduler));

    scheduler->cache = cache_create(cap);

    scheduler->sched_jobs = calloc(1, sizeof(JobPriorityHeap));
    scheduler->sched_jobs_w = calloc(1, sizeof(JobPriorityHeap));

    priority_heap_init(scheduler->sched_jobs, cap);
    priority_heap_init(scheduler->sched_jobs_w, cap);
    freelist_init(&scheduler->jobs, cap);

    scheduler->running = true;

    scheduler->waker = (pthread_cond_t)PTHREAD_COND_INITIALIZER;
    scheduler->mutex = (pthread_mutex_t)PTHREAD_MUTEX_INITIALIZER;
    scheduler->jobs_swap_lock = (pthread_rwlock_t)PTHREAD_RWLOCK_INITIALIZER;
    scheduler->jobs_grow_lock = (pthread_rwlock_t)PTHREAD_RWLOCK_INITIALIZER;

    return scheduler;
}

void scheduler_destroy(Scheduler* scheduler) {
    pthread_mutex_destroy(&scheduler->mutex);
    pthread_cond_destroy(&scheduler->waker);
    pthread_rwlock_destroy(&scheduler->jobs_swap_lock);
    pthread_rwlock_destroy(&scheduler->jobs_grow_lock);

    cache_destroy(scheduler->cache);
    priority_heap_destroy(scheduler->sched_jobs);
    priority_heap_destroy(scheduler->sched_jobs_w);
    freelist_destroy(&scheduler->jobs);
    free(scheduler->sched_jobs);
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
        scheduler_job_notify(data);
        return;
    }

#ifdef DEBUG
    // protocol_debug_print_request(req);
#endif

    const uint64_t block_size = 1024;

    struct Job job = {
        .data = data,
        .block_size = block_size,
        .block_count = ((req->end - req->start) + (block_size - 1)) / block_size,
    };

    memcpy(&job.req, req, sizeof(struct ProtocolRequest));

    uint32_t idx;

    if (scheduler->jobs.free == 0) {
        SCHEDULER_WRITE_JOBS(scheduler)
        idx = freelist_insert(&scheduler->jobs, job);
        SCHEDULER_WRITE_JOBS_END(scheduler)
    } else {
        idx = freelist_insert(&scheduler->jobs, job);
    }

    // Ensure writer buffer is up-to-date.
    priority_heap_copy(scheduler->sched_jobs_w, scheduler->sched_jobs);
    // Update writer buffer.
    priority_heap_insert(scheduler->sched_jobs_w, &idx, req->priority);

    // Perform swap under lock.
    SCHEDULER_WRITE_PRIO(scheduler)
    JobPriorityHeap* tmp = scheduler->sched_jobs;
    scheduler->sched_jobs = scheduler->sched_jobs_w;
    scheduler->sched_jobs_w = tmp;
    scheduler->sched_jobs_version++;
    SCHEDULER_WRITE_PRIO_END(scheduler)

    pthread_cond_broadcast(&scheduler->waker);
}

bool scheduler_schedule(
    Scheduler* scheduler, JobPriorityHeap* local_sched_jobs, uint32_t* local_sched_jobs_version, uint32_t* job_idx,
    uint64_t* start, uint64_t* end
) {
    if (scheduler->sched_jobs_version > *local_sched_jobs_version) {
        SCHEDULER_READ_PRIO(scheduler)
        priority_heap_copy(local_sched_jobs, scheduler->sched_jobs);
        *local_sched_jobs_version = scheduler->sched_jobs_version;
        SCHEDULER_READ_PRIO_END(scheduler)
    }

    *job_idx = UINT32_MAX;

    while (1) {
        const PriorityHeapNode(uint32_t)* node = (void*)priority_heap_get_max(local_sched_jobs);

        if (node == NULL) {
            break;
        }

        *job_idx = node->elem;

        SCHEDULER_READ_JOBS(scheduler)
        const struct Job* job = &scheduler->jobs.data[*job_idx];

        if (!scheduler_job_is_done(job)) {
            const uint64_t block_idx = atomic_fetch_add(&job->block_idx, 1);
            *start = job->req.start + block_idx * job->block_size;
            *end = MIN(job->req.end, *start + job->block_size);

            return true;
        }
        SCHEDULER_READ_JOBS_END(scheduler)

        priority_heap_pop_max(local_sched_jobs);
    }

    // Wait for new jobs.
    pthread_mutex_lock(&scheduler->mutex);
    pthread_cond_wait(&scheduler->waker, &scheduler->mutex);
    pthread_mutex_unlock(&scheduler->mutex);

    return false;
}

void scheduler_job_done(Scheduler* scheduler, const uint32_t job_idx, const uint64_t answer) {
    struct JobData* data = NULL;

    SCHEDULER_READ_JOBS(scheduler)
    struct Job* job = &scheduler->jobs.data[job_idx];
    job->block_idx = job->block_count;
    job->data->answer = answer;

    // Store cache entry.
    cache_insert_pending(scheduler->cache, job->req.hash, answer);
    data = job->data;
    SCHEDULER_READ_JOBS_END(scheduler)

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
