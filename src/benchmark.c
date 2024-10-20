#include "benchmark.h"
#include "cache.h"
#include "hash.h"
#include "prng.h"
#include "radix_tree.h"
#include "sha256/sha256.h"
#include "sha256/x4/sha256x4.h"
#include "sha256/x4x2/sha256x4x2.h"
#include "sha256/x8/sha256x8.h"
#include "thread/futex.h"
#include "thread/scheduler.h"
#include "thread/worker.h"
#include "vec.h"

#include <sys/random.h>

static uint64_t BENCHMARK_PRNG_STATE = 5225682921121941594;

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

struct EntrySha256 {
    HashDigest key;
    uint64_t data;
};

struct EntryRandomKeyLength {
    uint8_t key[32];
    uint8_t length;
    uint64_t data;
};

void benchmark_random_sha256_entry(struct EntrySha256* entry) {
    entry->data = random_u64();
    sha256_custom(entry->key, (uint8_t*)&entry->data);
}

void benchmark_random_random_key_length_entry(struct EntryRandomKeyLength* entry) {
    entry->length = random_u64_in_range(1, sizeof(entry->key));
    for (int i = 0; i < entry->length; i++) {
        entry->key[i] = random_u64() % sizeof(entry->key);
    }
    entry->data = random_u64();
}

void debug_print_uint4(const uint8_t* data, const size_t len) {
    for (size_t i = 0; i < (len * RADIX_TREE_KEY_RATIO); i++) {
        printf("%01x", radix_tree_key_unpack(data, i));
    }
}

void debug_tree_stats(_RadixTreeBase* tree) {
    printf(
        "[Edges] cap: %u, free: %u, free: %f%%\n", tree->edges.cap, tree->edges.free,
        (float)tree->edges.free / tree->edges.cap * 100
    );

    printf(
        "[Branches4] cap: %u, free: %u, free: %f%%\n", tree->branches4.cap, tree->branches4.free,
        (float)tree->branches4.free / tree->branches4.cap * 100
    );

    printf(
        "[Branches8] cap: %u, free: %u, free: %f%%\n", tree->branches8.cap, tree->branches8.free,
        (float)tree->branches8.free / tree->branches8.cap * 100
    );

    printf(
        "[Branches16] cap: %u, free: %u, free: %f%%\n", tree->branches16.cap, tree->branches16.free,
        (float)tree->branches16.free / tree->branches16.cap * 100
    );

    printf(
        "[BranchesFull] cap: %u, free: %u, free: %f%%\n", tree->branches_full.cap, tree->branches_full.free,
        (float)tree->branches_full.free / tree->branches_full.cap * 100
    );

    printf(
        "[Strings] cap: %u, free: %u, free: %f%%\n", tree->strings.cap, tree->strings.free,
        (float)tree->strings.free / tree->strings.cap * 100
    );

    printf(
        "[Leaves] cap: %u, free: %u, free: %f%%\n", tree->leaves.cap, tree->leaves.free,
        (float)tree->leaves.free / tree->leaves.cap * 100
    );
}

void benchmark_sha256_radix_tree_lookup() {
    getrandom(&BENCHMARK_PRNG_STATE, sizeof(BENCHMARK_PRNG_STATE), 0);

    // BENCHMARK_PRNG_STATE = 5397888409141263140;

    const uint64_t N = 1000000;
    printf("Benchmarking sha256 radix tree with %lu elements with seed %lu\n", N, BENCHMARK_PRNG_STATE);

    RadixTree(uint64_t, SHA256_DIGEST_LENGTH) tree;
    radix_tree_create(&tree, N);

    struct EntrySha256* entries = calloc(N, sizeof(struct EntrySha256));

    for (int i = 0; i < N; i++) {
        benchmark_random_sha256_entry(&entries[i]);
    }

    D_BENCHMARK_TIME_START()
    for (int i = 0; i < N; i++) {
        radix_tree_insert(&tree, entries[i].key, SHA256_DIGEST_LENGTH, &entries[i].data);
    }
    D_BENCHMARK_TIME_END("cache insert")

    debug_tree_stats((_RadixTreeBase*)&tree);

    D_BENCHMARK_TIME_START()
    for (int i = 0; i < N; i++) {
        const uint64_t* ga;
        radix_tree_get(&tree, entries[i].key, SHA256_DIGEST_LENGTH, &ga);

        if (ga == NULL || *ga != entries[i].data) {
            printf("Index %d failed to get data, expected %lu, got %p\n", i, entries[i].data, ga);
        }
    }
    D_BENCHMARK_TIME_END("cache get")

    radix_tree_destroy(&tree);
    free(entries);
}

void benchmark_random_key_radix_tree_lookup() {
    getrandom(&BENCHMARK_PRNG_STATE, sizeof(BENCHMARK_PRNG_STATE), 0);

    const uint64_t N = 1000000;

    printf("Benchmarking random key length radix tree with %lu elements with seed %lu\n", N, BENCHMARK_PRNG_STATE);

    RadixTree(uint64_t, 32) tree;
    radix_tree_create(&tree, N);

    struct EntryRandomKeyLength* entries = calloc(N, sizeof(struct EntryRandomKeyLength));
    for (int i = 0; i < N; i++) {
        benchmark_random_random_key_length_entry(&entries[i]);
    }

    D_BENCHMARK_TIME_START()
    for (int i = 0; i < N; i++) {
        radix_tree_insert(&tree, entries[i].key, entries[i].length, &entries[i].data);
    }

    D_BENCHMARK_TIME_END("random cache insert")

    D_BENCHMARK_TIME_START()
    for (int i = 0; i < N; i++) {
        const uint64_t* ga;
        radix_tree_get(&tree, entries[i].key, entries[i].length, &ga);

        if (ga == NULL) {
            printf("Index %d failed to get data, expected %lu, got %p\n", i, entries[i].data, ga);
        }
    }
    D_BENCHMARK_TIME_END("random cache get")

    struct EntryRandomKeyLength* uninserted = calloc(N, sizeof(struct EntryRandomKeyLength));
    for (int i = 0; i < N; i++) {
        benchmark_random_random_key_length_entry(&uninserted[i]);
    }

    D_BENCHMARK_TIME_START()
    for (int i = 0; i < N; i++) {
        const uint64_t* ga;
        radix_tree_get(&tree, uninserted[i].key, uninserted[i].length, &ga);
    }
    D_BENCHMARK_TIME_END("random cache get (missed)")

    radix_tree_destroy(&tree);
    free(entries);
    free(uninserted);
}

void benchmark_manual_radix_tree() {
    getrandom(&BENCHMARK_PRNG_STATE, sizeof(BENCHMARK_PRNG_STATE), 0);

    const struct EntryRandomKeyLength entries[] = {
        {.key = {0xb7, 0x7e, 0x5b}, .length = 3, .data = 1}, {.key = {0xb7, 0x24, 0xae}, .length = 3, .data = 2},
        {.key = {0xb7, 0x03, 0x14}, .length = 3, .data = 3}, {.key = {0xb4, 0x17, 0x4e}, .length = 3, .data = 4},
        {.key = {0xb7, 0xee, 0x21}, .length = 3, .data = 5}, {.key = {0xb7}, .length = 1, .data = 69}
    };

    const uint64_t N = sizeof(entries) / sizeof(struct EntryRandomKeyLength);

    printf("Benchmarking random key length radix tree with %lu elements with seed %lu\n", N, BENCHMARK_PRNG_STATE);

    RadixTree(uint64_t, 10) tree;
    radix_tree_create(&tree, N);

    D_BENCHMARK_TIME_START()
    for (int i = 0; i < N; i++) {
        radix_tree_insert(&tree, entries[i].key, entries[i].length, &entries[i].data);
        radix_tree_debug(&tree, stdout);
        printf("--------------------------\n");
    }
    D_BENCHMARK_TIME_END("manual cache insert")

    D_BENCHMARK_TIME_START()
    for (int i = 0; i < N; i++) {
        const uint64_t* ga;
        radix_tree_get(&tree, entries[i].key, entries[i].length, &ga);
        if (ga == NULL || *ga != entries[i].data) {
            printf("Index %d failed to get data, expected %lu, got %p\n", i, entries[i].data, ga);
        }
    }
    D_BENCHMARK_TIME_END("manual cache get")

    radix_tree_destroy(&tree);
}