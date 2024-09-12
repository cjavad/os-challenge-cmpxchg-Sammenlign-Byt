#pragma once

#include <stdint.h>
#include <x86intrin.h>
#include <time.h>

#define SHA256_INPUT_LENGTH 8
#define SHA256_DIGEST_LENGTH 32
#define SHA256_GCC_OPT_ATTR __attribute__((flatten, optimize("unroll-loops")))

typedef uint8_t HashDigest[SHA256_DIGEST_LENGTH];
typedef uint8_t HashInput[SHA256_INPUT_LENGTH];

void sha256_openssl(HashDigest hash, const HashInput data);
void sha256_custom(HashDigest hash, const HashInput data);
void sha256_optim(HashDigest hash, const HashInput data);
void sha256_fused(HashDigest hash, const HashInput data);
void sha256_fullyfused(HashDigest hash, const HashInput data);

void sprintf_hash(char *str, const HashDigest hash);
void print_hash(const HashDigest hash);

#define BENCHMARK_START                                                        \
    struct timespec st, et;                                                    \
    uint64_t start, end;                                                       \
    clock_gettime(CLOCK_MONOTONIC, &st);                                       \
    start = __rdtsc();
#define BENCHMARK_END(name, iterations)                                        \
    end = __rdtsc();                                                           \
    clock_gettime(CLOCK_MONOTONIC, &et);                                       \
    printf("[%s]\nEach: %lu ts, %lu ns\nTotal: %lu ts - %lu ns count total for "    \
           "%lu iterations\n",                                                 \
           #name, (end - start) / iterations,                                  \
           (et.tv_nsec - st.tv_nsec) / iterations, (end - start),              \
           (et.tv_nsec - st.tv_nsec), (uint64_t)iterations);

#define SHA256_BENCHMARK_ITERATIONS 1000000
#define BENCHMARK_SHA256(func)                                                 \
    {                                                                          \
        HashDigest hash;                                                       \
        BENCHMARK_START                                                        \
        for (uint64_t i = 0; i < SHA256_BENCHMARK_ITERATIONS; i++) {           \
            (func)(hash, (uint8_t *)&i);                                       \
        }                                                                      \
        BENCHMARK_END(func, SHA256_BENCHMARK_ITERATIONS)                       \
        printf("Hash: ");                                                      \
        print_hash(hash);                                                      \
        printf("\n");                                                          \
    }

#define BENCHMARK_SHA256_ALL                                                   \
    BENCHMARK_SHA256(sha256_openssl)                                           \
    BENCHMARK_SHA256(sha256_custom)                                            \
    BENCHMARK_SHA256(sha256_optim)                                             \
    BENCHMARK_SHA256(sha256_fused)                                             \
    BENCHMARK_SHA256(sha256_fullyfused)
