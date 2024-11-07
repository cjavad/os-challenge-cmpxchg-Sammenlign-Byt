#include "sched_priority.h"

#include "../bits/minmax.h"
#include "../cache.h"
#include "../sha256/sha256.h"

struct PriorityScheduler* scheduler_priority_create(const uint32_t default_cap) {
    struct PriorityScheduler* scheduler = calloc(1, sizeof(struct PriorityScheduler));

    freelist_init(&scheduler->jobs, default_cap);

    scheduler->jobs_r = calloc(1, sizeof(IndexPriorityHeap));
    scheduler->jobs_w = calloc(1, sizeof(IndexPriorityHeap));

    priority_heap_init(scheduler->jobs_r, default_cap);
    priority_heap_init(scheduler->jobs_w, default_cap);

    spin_rwlock_init(&scheduler->rlock);
    spin_rwlock_init(&scheduler->wlock);

    scheduler_base_init(&scheduler->base, default_cap);

    return scheduler;
}

void scheduler_priority_destroy(struct PriorityScheduler* scheduler) {
    priority_heap_destroy(scheduler->jobs_r);
    priority_heap_destroy(scheduler->jobs_w);
    freelist_destroy(&scheduler->jobs);
    free(scheduler->jobs_r);
    free(scheduler->jobs_w);
    scheduler_base_destroy(&scheduler->base);
    free(scheduler);
}

SchedulerJobId scheduler_priority_submit(
    struct PriorityScheduler* scheduler, const struct ProtocolRequest* request, struct SchedulerJobRecipient* recipient
) {
    // Cache hit/miss code is duplicated for now, as it is really simple.
    cache_process_pending(scheduler->base.cache);

    const uint64_t* cached_answer;

    radix_tree_get(&scheduler->base.cache->tree, request->hash, SHA256_DIGEST_LENGTH, &cached_answer);

    if (cached_answer != NULL) {
        scheduler_job_notify_recipient(recipient, *cached_answer);
        return SCHEDULER_NO_JOB_ID_SENTINEL;
    }

    // Submit new job to scheduler. (Starts at 1)
    const SchedulerJobId next_id = scheduler->base.job_id + 1;
    const uint64_t block_size = 1024;
    const uint64_t difficulty = request->end - request->start;

    struct PrioritySchedulerJob job = {
        .recipient = recipient,
        .block_size = block_size,
        .block_count = (difficulty + (block_size - 1)) / block_size,
        .id = next_id
    };

    memcpy(&job.request, request, sizeof(struct ProtocolRequest));

    // If the freelist needs to be grown (realloc)
    // we need to lock the read lock as the writer.
    const bool will_grow = scheduler->jobs.free == 0;

    if (will_grow) {
        spin_rwlock_wrlock(&scheduler->rlock);
    }

    const uint32_t job_idx = freelist_insert(&scheduler->jobs, job);

    if (will_grow) {
        spin_rwlock_wrunlock(&scheduler->rlock);
    }

    // Copy previous heap to new heap to preserve previous job.
    priority_heap_copy(scheduler->jobs_w, scheduler->jobs_r);

    // Cleanup all completed jobs from the write heap and jobs list
    for (uint32_t i = 0; i < scheduler->jobs_w->len; i++) {
        const uint32_t some_job_idx = scheduler->jobs_w->data[i].elem;
        const struct PrioritySchedulerJob* some_job = &scheduler->jobs.data[some_job_idx];

        if (scheduler_priority_job_is_done(some_job) && atomic_load(&some_job->rc) == 0) {
            freelist_remove(&scheduler->jobs, some_job_idx);
            priority_heap_remove(scheduler->jobs_w, i);
            i--;
        }
    }

    // Update write heap with the newest index
    priority_heap_insert(scheduler->jobs_w, &job_idx, request->priority);

    // Swap read and write heaps
    // TODO: Could this be an compare and exchange?
    spin_rwlock_wrlock(&scheduler->wlock);
    IndexPriorityHeap* tmp = scheduler->jobs_r;
    scheduler->jobs_r = scheduler->jobs_w;
    scheduler->jobs_w = tmp;
    // Increment job_id (We are a single writer so this is fine)
    atomic_store(&scheduler->base.job_id, next_id);
    spin_rwlock_wrunlock(&scheduler->wlock);

    // Wake up workers
    scheduler_wake_workers(&scheduler->base);

    return next_id;
}

void scheduler_priority_cancel(const struct PriorityScheduler* scheduler, const SchedulerJobId job_id) {
    for (uint32_t i = 0; i < scheduler->jobs_r->len; i++) {
        struct PrioritySchedulerJob* job = &scheduler->jobs.data[scheduler->jobs_r->data[i].elem];

        if (job->id != job_id) {
            continue;
        }

        // Mark job as done.
        atomic_store(&job->block_idx, job->block_count);
    }
}

bool scheduler_priority_schedule(
    struct PriorityScheduler* scheduler, IndexPriorityHeap* local_jobs, SchedulerJobId* prev_max_job_id,
    SchedulerJobId* prev_job_id, HashDigest target_hash, uint32_t* job_idx, uint64_t* start, uint64_t* end
) {

    // First attempt to get the newest version of the jobs heap (if the job id is newer)
    spin_rwlock_rdlock(&scheduler->wlock);
    if (scheduler->base.job_id > *prev_max_job_id) {
        priority_heap_copy(local_jobs, scheduler->jobs_r);
        *prev_max_job_id = scheduler->base.job_id;
    }
    spin_rwlock_rdunlock(&scheduler->wlock);

    *job_idx = SCHEDULER_NO_JOB_ID_SENTINEL;

    while (true) {
        const IndexPriorityHeapNode* node;
        priority_heap_get_max(local_jobs, (void*)&node);

        if (node == NULL) {
            break;
        }

        *job_idx = node->elem;

        spin_rwlock_rdlock(&scheduler->rlock);

        struct PrioritySchedulerJob* job = &scheduler->jobs.data[*job_idx];
        const uint64_t block_idx = atomic_fetch_add(&job->block_idx, 1);

        if (block_idx <= job->block_count) {
            *start = job->request.start + block_idx * job->block_size;
            *end = min(job->request.end, *start + job->block_size);

            // Update target hash only when the target job id changes to
            // minimize overhead.
            if (*prev_job_id != job->id) {
                memcpy(target_hash, job->request.hash, SHA256_DIGEST_LENGTH);
                *prev_job_id = job->id;
            }

            spin_rwlock_rdunlock(&scheduler->rlock);
            return true;
        }

        spin_rwlock_rdunlock(&scheduler->rlock);

        priority_heap_extract_max(local_jobs, NULL);
    }

    return false;
}

void* scheduler_priority_worker(struct PriorityScheduler* scheduler) {
    // Each thread holds local copies of data it works on for optimal performance.
    HashDigest target_hash = {0};
    // Job id starts at 1
    SchedulerJobId prev_job_id = 0;
    SchedulerJobId prev_max_job_id = 0;
    IndexPriorityHeap local_jobs = {0};

    while (scheduler->base.running) {
        uint32_t job_idx;
        uint64_t start;
        uint64_t end;

        // Try to get the next job to work on.
        if (!scheduler_priority_schedule(
                scheduler, &local_jobs, &prev_max_job_id, &prev_job_id, target_hash, &job_idx, &start, &end
            )) {
            // Wait until a new job is available.
            scheduler_park_worker(&scheduler->base, prev_max_job_id);
            continue;
        }

        scheduler_priority_enter_job(scheduler, job_idx);

        const uint64_t answer = reverse_sha256_x4(start, end, target_hash);

        if (answer == SCHEDULER_NO_ANSWER_SENTINEL) {
            scheduler_priority_leave_job(scheduler, job_idx);
            continue;
        }

        // TODO: Two threads can race on same recipient (double work is being performed)
        // Answer was found, extract recipient ptr and mark job as done.
        spin_rwlock_rdlock(&scheduler->rlock);
        struct PrioritySchedulerJob* job = &scheduler->jobs.data[job_idx];
        struct SchedulerJobRecipient* recipient = job->recipient;
        job->recipient = NULL;
        // Send answer to recipient and store in cache.
        scheduler_job_notify_recipient(recipient, answer);
        scheduler_priority_job_mark_as_done(job);
        spin_rwlock_rdunlock(&scheduler->rlock);

        cache_insert_pending(scheduler->base.cache, target_hash, answer);
    }

    return NULL;
}