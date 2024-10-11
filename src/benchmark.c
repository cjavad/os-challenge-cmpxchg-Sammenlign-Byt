#include "benchmark.h"

#include "cache.h"
#include "hash.h"
#include "prng.h"
#include "sha256/sha256.h"
#include "sha256/x4/sha256x4.h"
#include "sha256/x4x2/sha256x4x2.h"
#include "sha256/x8/sha256x8.h"
#include "thread/futex.h"
#include "thread/scheduler.h"
#include "thread/worker.h"
#include "vec.h"

#include <sys/random.h>

static uint64_t BENCHMARK_PRNG_STATE = 0x3;

uint64_t random_u64() { return xorshift64_prng_next(&BENCHMARK_PRNG_STATE); }

uint64_t random_u64_in_range(const uint64_t start, const uint64_t end) { return start + random_u64() % (end - start); }

void benchmark_random_req(ProtocolRequest* req, const bool worst_case) {
    req->start = random_u64();
    req->end = req->start + 30000000;

    const uint64_t data = worst_case ? req->end : random_u64_in_range(req->start, req->end);

    sha256_custom(req->hash, (uint8_t*)&data);

    req->priority = 1;
}

void benchmark_hash() {
    fprintf(stderr, "Benchmarking sha256\n");

    D_BENCHMARK_TIME_START()

        BENCHMARK_SHA256_ALL
        // BENCHMARK_SHA256X8_ALL
        BENCHMARK_SHA256X4_ALL
        // BENCHMARK_SHA256X4(sha256x4_asm)
        // BENCHMARK_SHA256X4X2_ALL

        // BENCHMARK_REVERSE_HASH_ALL(0, 30000001, 30000000)

        // BENCHMARK_SHA256X4(sha256x4_cyclic)
        // printf("warmup done\n");

        // BENCHMARK_SHA256(sha256_optim)
        // BENCHMARK_SHA256X4(sha256x4_optim)
        // BENCHMARK_SHA256X4X2(sha256x4x2_optim)
        // BENCHMARK_SHA256X8(sha256x8_optim)

    D_BENCHMARK_TIME_END("sha256")
}

void benchmark_reference_block_time(ProtocolRequest* req) {
    printf("single thread reference block time\n");
    protocol_debug_print_request(req);
    D_BENCHMARK_TIME_START()
        uint64_t s = reverse_hash_x4(req->start, req->end, req->hash);
        printf("answer: %lu\n", s);
    D_BENCHMARK_TIME_END("sha256 solve reference")
}

void benchmark_scheduler() {
    // Randomly seed the PRNG
    // getrandom(&BENCHMARK_PRNG_STATE, sizeof(BENCHMARK_PRNG_STATE), 0);

    Scheduler* scheduler = scheduler_create(8);

    const size_t count = 8;

    ProtocolRequest reqs[count];
    JobData** data = calloc(count, sizeof(JobData*));

    for (int i = 0; i < count; i++) {
        benchmark_random_req(&reqs[i], true);

        reqs[i].priority = (count - i) % 4;

        data[i] = scheduler_create_job_data(JOB_TYPE_FUTEX, 0);
        scheduler_submit(scheduler, &reqs[i], data[i]);
    }

    benchmark_reference_block_time(&reqs[count - 1]);

    WorkerPool* pool = worker_create_pool(cpu_core_count(), scheduler);

    D_BENCHMARK_TIME_START()
        for (int i = 0; i < count; i++) {
            futex_wait(&data[i]->futex);
        }
    D_BENCHMARK_TIME_END("scheduler")

    free(data);
    worker_destroy_pool(pool);
    scheduler_destroy(scheduler);
}

struct Entry {
    HashDigest key;
    uint64_t data;
};

void benchmark_random_entry(struct Entry* entry) {
    entry->data = random_u64();
    sha256_custom(entry->key, (uint8_t*)&entry->data);
}

void benchmark_test_vec() {
    getrandom(&BENCHMARK_PRNG_STATE, sizeof(BENCHMARK_PRNG_STATE), 0);

    const uint64_t N = 10000000;
    Cache* cache = cache_create(N);

    struct Entry* entries = calloc(N, sizeof(struct Entry));

    for (int i = 0; i < N; i++) {
        benchmark_random_entry(&entries[i]);
    }

    D_BENCHMARK_TIME_START()
        for (int i = 0; i < N; i++) {
            cache_insert(cache, entries[i].key, entries[i].data);
        }
    D_BENCHMARK_TIME_END("cache insert")

    uint32_t max_idx = 0;
    uint32_t max_len = 0;

    for (uint32_t idx = 0; idx < cache->edges.cap; idx++) {
        struct TreeNodeEdge* edge = &cache->edges.data[idx];

        if (edge->next.type != TT_LEAF && edge->length > max_len) {
            max_idx = idx;
            max_len = edge->length;
        }
    }

    printf("Max edge length: %d\n", max_len);
    cache_debug_print_node(cache, &(TreeNodePointer){.type = TT_EDGE, .idx = max_idx}, 0);

    D_BENCHMARK_TIME_START()
        for (int i = 0; i < N; i++) {
            const uint64_t g = cache_get(cache, entries[i].key);

            if (g != entries[i].data) {
                printf("Index %d failed to get data, expected %lu, got %lu\n", i, entries[i].data, g);
            }
        }
    D_BENCHMARK_TIME_END("cache get")
}