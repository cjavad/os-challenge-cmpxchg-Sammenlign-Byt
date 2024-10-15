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
    uint8_t key[255];
    uint8_t length;
    uint64_t data;
};

void benchmark_random_sha256_entry(struct EntrySha256* entry) {
    entry->data = random_u64();
    sha256_custom(entry->key, (uint8_t*)&entry->data);
}

void benchmark_random_random_key_length_entry(struct EntryRandomKeyLength* entry) {
    entry->length = random_u64_in_range(1, 255);
    for (int i = 0; i < entry->length; i++) {
        entry->key[i] = random_u64() % 256;
    }
    entry->data = random_u64();
}

void benchmark_radix_tree_stats(const _RadixTreeBase* tree, const uint8_t key_length) {
    printf(
        "[Strings] cap: %d, free: %d usage %f%%\n", tree->strings.cap, tree->strings.free,
        (float)tree->strings.free / (float)tree->strings.cap * 100
    );
    printf(
        "[Edges] cap: %d, free: %d usage %f%%\n", tree->edges.cap, tree->edges.free,
        (float)tree->edges.free / (float)tree->edges.cap * 100
    );
    printf(
        "[Branches] cap: %d, free: %d usage %f%%\n", tree->branches.cap, tree->branches.free,
        (float)tree->branches.free / (float)tree->branches.cap * 100
    );
    printf(
        "[Leaf] cap: %d, free: %d usage %f%%\n", tree->leaves.cap, tree->leaves.free,
        (float)tree->leaves.free / (float)tree->leaves.cap * 100
    );

    uint32_t used_strs = 0;

    for (uint32_t i = 0; i < tree->strings.cap; i += key_length) {
        // ignore all zero
        int sum = 0;

        for (int k = 0; k < key_length; k++) {
            sum += tree->strings.data[i + k];
        }

        if (sum == 0) {
            continue;
        }

        used_strs++;
    }

    printf("[Strings] used: %d\n", used_strs);
}

void debug_print_uint4(const uint8_t* data, const size_t len) {
    for (size_t i = 0; i < (len * RADIX_TREE_KEY_RATIO); i++) {
        printf("%01x", radix_tree_key_unpack(data, i));
    }
    printf("\n");
}

void benchmark_sha256_radix_tree_lookup() {
    getrandom(&BENCHMARK_PRNG_STATE, sizeof(BENCHMARK_PRNG_STATE), 0);

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

    benchmark_radix_tree_stats((_RadixTreeBase*)&tree, SHA256_DIGEST_LENGTH);

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

    // BENCHMARK_PRNG_STATE = 8111887784597664444;
    const uint64_t N = 100000;

    printf("Benchmarking random key length radix tree with %lu elements with seed %lu\n", N, BENCHMARK_PRNG_STATE);

    RadixTree(uint64_t, 255) tree;
    radix_tree_create(&tree, N);

    struct EntryRandomKeyLength* entries = calloc(N, sizeof(struct EntryRandomKeyLength));
    for (int i = 0; i < N; i++) {
        benchmark_random_random_key_length_entry(&entries[i]);
    }

    // Find all duplicate keys and set length to 0
    for (int i = 0; i < N; i++) {
        for (int j = i + 1; j < N; j++) {
            if (entries[i].length == entries[j].length &&
                memcmp(entries[i].key, entries[j].key, entries[i].length) == 0) {
                entries[j].length = 0;
            }
        }
    }

    // struct EntryRandomKeyLength entries[] = {
    //     {.key = {0xb8, 0x71, 0x8a}, .length = 3},
    //     {.key = {0xb8, 0xae, 0x70}, .length = 3},
    //     {.key = {0xb8}, .length = 1},
    // };

    // Print all entries key: value
    for (int i = 0; i < N; i++) {
        // printf("Entry %d: ", i);
        // debug_print_uint4(entries[i].key, entries[i].length);
        // printf("Value: %lu\n", entries[i].data);
    }

    D_BENCHMARK_TIME_START()
    for (int i = 0; i < N; i++) {
        radix_tree_insert(&tree, entries[i].key, entries[i].length, &entries[i].data);
    }
    D_BENCHMARK_TIME_END("random cache insert")

    // radix_tree_debug(&tree, stdout);
    benchmark_radix_tree_stats((_RadixTreeBase*)&tree, 255);

    D_BENCHMARK_TIME_START()
    for (int i = 0; i < N; i++) {
        if (entries[i].length == 0) {
            continue;
        }

        const uint64_t* ga;
        radix_tree_get(&tree, entries[i].key, entries[i].length, &ga);

        if (ga == NULL || *ga != entries[i].data) {
            printf("Index %d failed to get data, expected %lu, got %p\n", i, entries[i].data, ga);
            exit(60);
        }
    }
    D_BENCHMARK_TIME_END("random cache get")

    radix_tree_destroy(&tree);
    free(entries);
}