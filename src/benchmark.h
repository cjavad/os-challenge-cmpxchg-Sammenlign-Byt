#pragma once

#include <stdint.h>

#include "sort.h"

#include <stdlib.h>

#define D_BENCHMARK_START(increment, samples)\
{\
    uint32_t inc = increment;\
    uint32_t benchmark_count = samples / inc;\
    uint16_t* benchmark_samples = malloc(benchmark_count * sizeof(uint16_t));

#define D_BENCHMARK_WARMUP(count)\
    for (uint32_t i = 0; i < count; i+=inc)

#define D_BENCHMARK_LOOP_START()\
    for (uint32_t i = 0; i < benchmark_count * inc; i+=inc) {\
        uint64_t start = __rdtsc();

#define D_BENCHMARK_LOOP_END()\
        uint64_t end = __rdtsc();\
        benchmark_samples[i / inc] = end-start;\
    }

#define D_BENCHMARK_END()\
    uint16_t* buffer = malloc(benchmark_count * sizeof(uint16_t));\
    radix_sort16(benchmark_samples, benchmark_samples, buffer, benchmark_count);\
    uint32_t index_start = benchmark_count / 10;\
    uint32_t index_end = benchmark_count - (benchmark_count / 10);\
    uint64_t total_count = (index_end - index_start) * inc;\
    uint64_t sum = 0; for (uint32_t i = index_start; i < index_end; i++) sum += benchmark_samples[i];\
    uint64_t per = sum / total_count;\
    printf("benchmark took total of %lu for %lu iterations, avg: %lu\n", sum, total_count, per);\
    free(benchmark_samples);\
    free(buffer);\
}