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

static uint64_t BENCHMARK_PRNG_STATE = 0xdeadbeef;

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

void benchmark_test_vec() {
    Cache* cache = cache_create();
    cache_debug_print(cache);

    HashDigest key1;
    uint64_t data1 = 1;
    HashDigest key2;
    uint64_t data2 = 2;
    HashDigest key3;
    uint64_t data3 = 3;

    sha256_custom(key1, (uint8_t*)&data1);
    sha256_custom(key2, (uint8_t*)&data2);
    sha256_custom(key3, (uint8_t*)&data3);

    cache_insert(cache, key1, data1);
    cache_debug_print(cache);

    cache_insert(cache, key2, data2);
    cache_debug_print(cache);

    cache_insert(cache, key3, data3);
    cache_debug_print(cache);

    printf("Get key1: %lu\n", cache_get(cache, key1));
    printf("Get key2: %lu\n", cache_get(cache, key2));
    printf("Get key3: %lu\n", cache_get(cache, key3));
}