#pragma once

#include <stdint.h>
#include <time.h>
#include <x86intrin.h>
#include "../benchmark.h"

#define SHA256_INPUT_LENGTH 8
#define SHA256_DIGEST_LENGTH 32
#define SHA256_GCC_OPT_ATTR __attribute__((flatten, optimize("unroll-loops")))

typedef uint8_t HashDigest[SHA256_DIGEST_LENGTH];
typedef uint8_t HashInput[SHA256_INPUT_LENGTH];

void sha256_custom(HashDigest hash, const HashInput data);
void sha256_optim(HashDigest hash, const HashInput data);
void sha256_fused(HashDigest hash, const HashInput data);
void sha256_fullyfused(HashDigest hash, const HashInput data);

void sprintf_hash(char *str, const HashDigest hash);
void print_hash(const HashDigest hash);

#define BENCHMARK_TIMESTAMP_DIFF(et, st)                                       \
    ((et.tv_sec - st.tv_sec) * 1000000000 + (et.tv_nsec - st.tv_nsec))

#define BENCHMARK_START                                                        \
    struct timespec st, et;                                                    \
    uint64_t start, end;                                                       \
    clock_gettime(CLOCK_REALTIME, &st);                                        \
    start = __rdtsc();
#define BENCHMARK_END(name, iterations)                                        \
    end = __rdtsc();                                                           \
    clock_gettime(CLOCK_REALTIME, &et);                                        \
    printf(                                                                    \
        "[%s]\nEach: %lu ts, %lu ns\nTotal: %lu ts - %lu ns count total for "  \
        "%lu iterations\n",                                                    \
        #name, (end - start) / iterations,                                     \
        BENCHMARK_TIMESTAMP_DIFF(et, st) / iterations, (end - start),          \
        BENCHMARK_TIMESTAMP_DIFF(et, st), (uint64_t)iterations);

#define SHA256_BENCHMARK_ITERATIONS 30000000
// #define BENCHMARK_SHA256(func)                                                 \
//     {                                                                          \
//         HashDigest hash;                                                       \
//         BENCHMARK_START                                                        \
//         for (uint64_t i = 0; i < SHA256_BENCHMARK_ITERATIONS; i++) {           \
//             (func)(hash, (uint8_t *)&i);                                       \
//         }                                                                      \
//         BENCHMARK_END(func, SHA256_BENCHMARK_ITERATIONS)                       \
//         printf("Hash: ");                                                      \
//         print_hash(hash);                                                      \
//         printf("\n");                                                          \
//     }

#define BENCHMARK_SHA256(func)\
printf("benchmarking %s\n", #func);\
D_BENCHMARK_START(1, SHA256_BENCHMARK_ITERATIONS)\
        uint8_t hash[SHA256_DIGEST_LENGTH] __attribute__((aligned(16)));\
        uint64_t data[1] __attribute__((aligned(16))) = {0};\
        D_BENCHMARK_WARMUP(1000000) {\
            (func)(hash, (uint8_t*)data);\
        }\
        D_BENCHMARK_LOOP_START()\
            (func)(hash, (uint8_t*)data);\
            data[0] += 1;\
        D_BENCHMARK_LOOP_END()\
D_BENCHMARK_END()\

#define BENCHMARK_SHA256_ALL                                                   \
    BENCHMARK_SHA256(sha256_custom)                                            \
    BENCHMARK_SHA256(sha256_optim)                                             \
    BENCHMARK_SHA256(sha256_fused)                                             \
    BENCHMARK_SHA256(sha256_fullyfused)
