#include "benchmark.h"

#include "../bits/futex.h"
#include "../bits/prng.h"
#include "../bits/radix_tree.h"
#include "../protocol.h"
#include "../scheduler/sched_priority.h"
#include "../server/worker_pool.h"
#include "../sha256/sha256.h"
#include "../sha256/x1/benchmark.h"
#include "../sha256/x4/benchmark.h"
#include "../sha256/x4x2/benchmark.h"
#include "../sha256/x8/benchmark.h"

#include <sys/random.h>
#include <time.h>

static uint64_t BENCHMARK_PRNG_STATE = 5225682921121941594;

uint64_t random_u64() { return xorshift64_prng_next(&BENCHMARK_PRNG_STATE); }

uint64_t random_u64_in_range(
    const uint64_t start,
    const uint64_t end
) {
    return start + random_u64() % (end - start);
}

void benchmark_random_req(
    struct ProtocolRequest* req,
    const bool worst_case
) {
    req->start = random_u64();
    req->end = req->start + 30000000;

    const uint64_t data =
        worst_case ? req->end : random_u64_in_range(req->start, req->end);

    sha256_custom(req->hash, (uint8_t*)&data);

    req->priority = 1;
}

void benchmark_hash() {
    fprintf(stderr, "Benchmarking sha256\n");

    D_BENCHMARK_TIME_START()

    BENCHMARK_SHA256X1_ALL
    // BENCHMARK_SHA256X8_ALL
    BENCHMARK_SHA256X4_ALL
    // BENCHMARK_SHA256X4(sha256x4_asm)
    BENCHMARK_SHA256X4X2_ALL

    D_BENCHMARK_TIME_END("sha256 (all)")
}

#define BENCHMARK_REVERSE_ITERATIONS 1000
#define BENCHMARK_REVERSE_FUNC(func)                                           \
    {                                                                          \
        printf("benchmarking %s\n", #func);                                    \
        D_BENCHMARK_START(1, BENCHMARK_REVERSE_ITERATIONS)                     \
        uint8_t target[SHA256_DIGEST_LENGTH] __attribute__((aligned(16)));     \
        uint64_t data __attribute__((aligned(16))) = 100000;                   \
        sha256_custom(target, (uint8_t*)&data);                                \
        D_BENCHMARK_WARMUP(200) { (func)(1, 30000000, target); }               \
        D_BENCHMARK_LOOP_START()                                               \
        (func)(1, 30000000, target);                                           \
        D_BENCHMARK_LOOP_END()                                                 \
        D_BENCHMARK_END()                                                      \
    }

void benchmark_reverse() {
    D_BENCHMARK_TIME_START()
    BENCHMARK_REVERSE_FUNC(reverse_sha256x4_fullyfused_asm)
    D_BENCHMARK_TIME_END("sha256 reverse x4 fullyfused asm")

    D_BENCHMARK_TIME_START()
    BENCHMARK_REVERSE_FUNC(reverse_sha256x4_fullyfused_asm_2)
    D_BENCHMARK_TIME_END("sha256 reverse x4 fullyfused asm 2")


    D_BENCHMARK_TIME_START()
    BENCHMARK_REVERSE_FUNC(reverse_sha256x4_fullyfused)
    D_BENCHMARK_TIME_END("sha256 reverse x4 fullyfused")

    D_BENCHMARK_TIME_START()
    BENCHMARK_REVERSE_FUNC(reverse_sha256_x4)
    D_BENCHMARK_TIME_END("sha256 reverse x4")

    D_BENCHMARK_TIME_START()
    BENCHMARK_REVERSE_FUNC(reverse_sha256_x4x2orx8)
    D_BENCHMARK_TIME_END("sha256 reverse x4x2orx8")

    D_BENCHMARK_TIME_START()
    BENCHMARK_REVERSE_FUNC(reverse_sha256)
    D_BENCHMARK_TIME_END("sha256 reverse")
}

void benchmark_reference_block_time(
    const struct ProtocolRequest* req
) {
    printf("single thread reference block time\n");
    protocol_debug_print_request(req);
    D_BENCHMARK_TIME_START()
    const uint64_t s = reverse_sha256_x4(req->start, req->end, req->hash);
    printf("answer: %lu\n", s);
    D_BENCHMARK_TIME_END("sha256 solve reference")
}

void benchmark_scheduler() {
    // Randomly seed the PRNG
    // getrandom(&BENCHMARK_PRNG_STATE, sizeof(BENCHMARK_PRNG_STATE), 0);

    struct PriorityScheduler* scheduler = scheduler_priority_create(8);

    const uint64_t count = 8;

    struct ProtocolRequest reqs[count];
    struct SchedulerJobRecipient** data =
        calloc(count, sizeof(struct JobData*));

    for (uint64_t i = 0; i < count; i++) {
        benchmark_random_req(&reqs[i], true);

        reqs[i].priority = (count - i) % 4;

        data[i] = scheduler_create_job_recipient(
            SCHEDULER_JOB_RECIPIENT_TYPE_FUTEX,
            0
        );
        scheduler_priority_submit(scheduler, &reqs[i], data[i]);
    }

    benchmark_reference_block_time(&reqs[count - 1]);

    struct WorkerPool* pool = worker_create_pool(
        worker_pool_get_concurrency(),
        (void*)scheduler,
        (void* (*)(void*))scheduler_priority_worker
    );

    D_BENCHMARK_TIME_START()
    for (uint64_t i = 0; i < count; i++) {
        while (1) {

            if (__sync_val_compare_and_swap(&data[i]->futex, 1, 0)) {
                break;
            }

            futex(&data[i]->futex, FUTEX_WAIT, 0, NULL, NULL, 0);
        }

        free(data[i]);
    }
    D_BENCHMARK_TIME_END("scheduler")

    worker_destroy_pool(pool);
    scheduler_priority_destroy(scheduler);
    free(data);
}

struct EntrySha256 {
    HashDigest key;
    uint64_t data;
};

struct EntryRandomKeyLength {
    uint8_t key[32];
    uint8_t length;
    uint64_t data;
};

void benchmark_random_sha256_entry(
    struct EntrySha256* entry
) {
    entry->data = random_u64();
    sha256_custom(entry->key, (uint8_t*)&entry->data);
}

void benchmark_random_random_key_length_entry(
    struct EntryRandomKeyLength* entry
) {
    entry->length = random_u64_in_range(1, sizeof(entry->key));
    for (int i = 0; i < entry->length; i++) {
        entry->key[i] = random_u64() % sizeof(entry->key);
    }
    entry->data = random_u64();
}

void debug_print_uint4(
    const uint8_t* data,
    const size_t len
) {
    for (size_t i = 0; i < (len * RADIX_TREE_KEY_RATIO); i++) {
        printf("%01x", radix_tree_key_unpack(data, i));
    }
}

void benchmark_sha256_radix_tree_lookup() {
    // getrandom(&BENCHMARK_PRNG_STATE, sizeof(BENCHMARK_PRNG_STATE), 0);
    // BENCHMARK_PRNG_STATE = 5397888409141263140;

    const uint64_t N = 1000000;
    printf(
        "Benchmarking sha256 radix tree with %lu elements with seed %lu\n",
        N,
        BENCHMARK_PRNG_STATE
    );

    RadixTree(uint64_t, SHA256_DIGEST_LENGTH) tree;
    radix_tree_create(&tree, N);

    struct EntrySha256* entries = calloc(N, sizeof(struct EntrySha256));

    for (uint64_t i = 0; i < N; i++) {
        benchmark_random_sha256_entry(&entries[i]);
    }

    D_BENCHMARK_TIME_START()
    for (uint64_t i = 0; i < N; i++) {
        radix_tree_insert(
            &tree,
            entries[i].key,
            SHA256_DIGEST_LENGTH,
            &entries[i].data
        );
    }
    D_BENCHMARK_TIME_END("cache insert")

    D_BENCHMARK_TIME_START()
    for (uint64_t i = 0; i < N; i++) {
        const uint64_t* ga;
        radix_tree_get(&tree, entries[i].key, SHA256_DIGEST_LENGTH, &ga);

        if (ga == NULL || *ga != entries[i].data) {
            printf(
                "Index %lu failed to get data, expected %lu, got %p\n",
                i,
                entries[i].data,
                ga
            );
        }
    }
    D_BENCHMARK_TIME_END("cache get")

    radix_tree_destroy(&tree);
    free(entries);
}

void benchmark_random_key_radix_tree_lookup() {
    // getrandom(&BENCHMARK_PRNG_STATE, sizeof(BENCHMARK_PRNG_STATE), 0);

    const uint64_t N = 1000000;

    printf(
        "Benchmarking random key length radix tree with %lu elements with seed "
        "%lu\n",
        N,
        BENCHMARK_PRNG_STATE
    );

    RadixTree(uint64_t, 32) tree;
    radix_tree_create(&tree, N);

    struct EntryRandomKeyLength* entries =
        calloc(N, sizeof(struct EntryRandomKeyLength));
    for (uint64_t i = 0; i < N; i++) {
        benchmark_random_random_key_length_entry(&entries[i]);
    }

    D_BENCHMARK_TIME_START()
    for (uint64_t i = 0; i < N; i++) {
        radix_tree_insert(
            &tree,
            entries[i].key,
            entries[i].length,
            &entries[i].data
        );
    }

    D_BENCHMARK_TIME_END("random cache insert")

    D_BENCHMARK_TIME_START()
    for (uint64_t i = 0; i < N; i++) {
        const uint64_t* ga;
        radix_tree_get(&tree, entries[i].key, entries[i].length, &ga);

        if (ga == NULL) {
            printf(
                "Index %lu failed to get data, expected %lu, got %p\n",
                i,
                entries[i].data,
                ga
            );
        }
    }
    D_BENCHMARK_TIME_END("random cache get")

    struct EntryRandomKeyLength* uninserted =
        calloc(N, sizeof(struct EntryRandomKeyLength));
    for (uint64_t i = 0; i < N; i++) {
        benchmark_random_random_key_length_entry(&uninserted[i]);
    }

    D_BENCHMARK_TIME_START()
    for (uint64_t i = 0; i < N; i++) {
        const uint64_t* ga;
        radix_tree_get(&tree, uninserted[i].key, uninserted[i].length, &ga);
    }
    D_BENCHMARK_TIME_END("random cache get (missed)")

    radix_tree_destroy(&tree);
    free(entries);
    free(uninserted);
}

void benchmark_manual_radix_tree() {
    // getrandom(&BENCHMARK_PRNG_STATE, sizeof(BENCHMARK_PRNG_STATE), 0);

    const struct EntryRandomKeyLength entries[] = {
        {.key = {0xb7, 0x7e, 0x5b}, .length = 3, .data = 1},
        {.key = {0xb7, 0x24, 0xae}, .length = 3, .data = 2},
        {.key = {0xb7, 0x03, 0x14}, .length = 3, .data = 3},
        {.key = {0xb4, 0x17, 0x4e}, .length = 3, .data = 4},
        {.key = {0xb7, 0xee, 0x21}, .length = 3, .data = 5},
        {.key = {0xb7}, .length = 1, .data = 69}
    };

    const uint64_t N = sizeof(entries) / sizeof(struct EntryRandomKeyLength);

    printf(
        "Benchmarking random key length radix tree with %lu elements with seed "
        "%lu\n",
        N,
        BENCHMARK_PRNG_STATE
    );

    RadixTree(uint64_t, 10) tree;
    radix_tree_create(&tree, N);

    D_BENCHMARK_TIME_START()
    for (uint64_t i = 0; i < N; i++) {
        radix_tree_insert(
            &tree,
            entries[i].key,
            entries[i].length,
            &entries[i].data
        );
    }
    D_BENCHMARK_TIME_END("manual cache insert")

    D_BENCHMARK_TIME_START()
    for (uint64_t i = 0; i < N; i++) {
        const uint64_t* ga;
        radix_tree_get(&tree, entries[i].key, entries[i].length, &ga);
        if (ga == NULL || *ga != entries[i].data) {
            printf(
                "Index %lu failed to get data, expected %lu, got %p\n",
                i,
                entries[i].data,
                ga
            );
        }
    }
    D_BENCHMARK_TIME_END("manual cache get")

    radix_tree_destroy(&tree);
}

void benchmark() {
    benchmark_reverse();
    benchmark_hash();
    benchmark_scheduler();
    benchmark_sha256_radix_tree_lookup();
    benchmark_random_key_radix_tree_lookup();
    benchmark_manual_radix_tree();
}