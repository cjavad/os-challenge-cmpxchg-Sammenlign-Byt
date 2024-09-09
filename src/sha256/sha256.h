#pragma once

#include <stdint.h>
#include <x86intrin.h>

#define SHA256_DIGEST_LENGTH 32

typedef uint8_t HashDigest[SHA256_DIGEST_LENGTH];
typedef uint8_t HashInput[8];

void sha256_openssl(HashDigest hash, const HashInput data);
void sha256_custom(HashDigest hash, const HashInput data);

void sprintf_hash(char *str, const HashDigest hash);
void print_hash(const HashDigest hash);

#define BENCHMARK_ITERATIONS 1000000
#define BENCHMARK_SHA256(func, data)                                           \
    {                                                                          \
        HashDigest hash;                                                       \
        uint64_t start = __rdtsc();                                            \
        for (int i = 0; i < BENCHMARK_ITERATIONS; i++) {                       \
            (func)(hash, (data));                                              \
        }                                                                      \
        uint64_t end = __rdtsc();                                              \
        printf("%s: %lu ts count per - %lu ts count total for %lu iterations\n",   \
               #func, (end - start) / BENCHMARK_ITERATIONS, (end - start),     \
               (uint64_t)BENCHMARK_ITERATIONS);                                \
        printf("Hash: ");                                                      \
        print_hash(hash);                                                      \
        printf("\n");                                                          \
    }

#define BENCHMARK_ALL(data)                                                    \
    BENCHMARK_SHA256(sha256_openssl, (data))                                   \
    BENCHMARK_SHA256(sha256_custom, (data))
