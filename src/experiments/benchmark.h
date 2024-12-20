#pragma once
#include "../bits/sort.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>

#if defined(__i386__) || defined(__x86_64__)
#include <x86intrin.h>
#else
#define __rdtsc() 0
#endif

#define D_BENCHMARK_START(increment, samples)                                  \
    {                                                                          \
        uint32_t inc = increment;                                              \
        uint32_t benchmark_count = samples / inc;                              \
        uint16_t* benchmark_samples =                                          \
            malloc(benchmark_count * sizeof(uint16_t));

#define D_BENCHMARK_WARMUP(count) for (uint32_t i = 0; i < count; i += inc)

#define D_BENCHMARK_LOOP_START()                                               \
    for (uint32_t i = 0; i < benchmark_count * inc; i += inc) {                \
        uint64_t start = __rdtsc();

#define D_BENCHMARK_LOOP_END()                                                 \
    uint64_t end = __rdtsc();                                                  \
    benchmark_samples[i / inc] = end - start;                                  \
    }

#define D_BENCHMARK_END()                                                      \
    uint16_t* buffer = malloc(benchmark_count * sizeof(uint16_t));             \
    radix_sort16(                                                              \
        benchmark_samples,                                                     \
        benchmark_samples,                                                     \
        buffer,                                                                \
        benchmark_count                                                        \
    );                                                                         \
    uint32_t index_start = benchmark_count / 10;                               \
    uint32_t index_end = benchmark_count - (benchmark_count / 10);             \
    uint64_t total_count = (index_end - index_start) * inc;                    \
    uint64_t sum = 0;                                                          \
    for (uint32_t i = index_start; i < index_end; i++)                         \
        sum += benchmark_samples[i];                                           \
    uint64_t per = sum / total_count;                                          \
    printf(                                                                    \
        "benchmark took total of %lu for %lu iterations, avg: %lu\n",          \
        sum,                                                                   \
        total_count,                                                           \
        per                                                                    \
    );                                                                         \
    free(benchmark_samples);                                                   \
    free(buffer);                                                              \
    }

#define D_BENCHMARK_TIME_START()                                               \
    {                                                                          \
        struct timespec st, et;                                                \
        clock_gettime(CLOCK_REALTIME, &st);

#define D_BENCHMARK_TIME_END(name)                                             \
    clock_gettime(CLOCK_REALTIME, &et);                                        \
    printf(                                                                    \
        "[%s] %lu ns\n",                                                       \
        name,                                                                  \
        (et.tv_sec - st.tv_sec) * 1000000000 + (et.tv_nsec - st.tv_nsec)       \
    );                                                                         \
    }

#define SHA256_BENCHMARK_ITERATIONS 30000000

void benchmark_hash();
void benchmark_scheduler();
void benchmark_sha256_radix_tree_lookup();
void benchmark_random_key_radix_tree_lookup();
void benchmark_manual_radix_tree();
void benchmark();
