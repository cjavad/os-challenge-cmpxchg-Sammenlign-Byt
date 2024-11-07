#include "sort.h"

void radix_sort16(
    uint16_t* dest,
    uint16_t* src,
    uint16_t* restrict buffer,
    uint32_t length
) {
    uint64_t pcounts[256]; // 2 kB cache (32 lines)
    uint32_t* counts = (uint32_t*)pcounts;

    __builtin_memset(pcounts, 0, 256 * sizeof(uint64_t));

    // counting pass
    for (uint32_t i = 0; i < length; i++) {
        uint16_t v = src[i];
        counts[((v << 1) & 0x1fe) + 0]++;
        counts[((v >> 7) & 0x1fe) + 1]++;
    }

    for (uint32_t i = 0; i < 256 - 1; i++) {
        // pcounts[i + 1] += pcounts[i];
        counts[i * 2 + 2] += counts[i * 2];
        counts[i * 2 + 3] += counts[i * 2 + 1];
    }
    // sorting pass
    for (uint32_t i = length; i--;) {
        uint16_t v = src[i];
        uint32_t j = ((v << 1) & 0x1fe) + 0;
        buffer[--counts[j]] = v;
    }
    for (uint32_t i = length; i--;) {
        uint16_t v = buffer[i];
        uint32_t j = ((v >> 7) & 0x1fe) + 1;
        dest[--counts[j]] = v;
    }
}

void radix_sort32(
    uint32_t* dest,
    uint32_t* src,
    uint32_t* restrict buffer,
    uint32_t length
) {
    __uint128_t pcounts[256]; // 4 kB cache (64 lines)
    uint32_t* counts = (uint32_t*)pcounts;

    __builtin_memset(pcounts, 0, 256 * sizeof(__uint128_t));

    // counting pass
    for (uint32_t i = 0; i < length; i++) {
        uint32_t v = src[i];
        counts[((v << 2) & 0x3fc) + 0]++;
        counts[((v >> 6) & 0x3fc) + 1]++;
        counts[((v >> 14) & 0x3fc) + 2]++;
        counts[((v >> 22) & 0x3fc) + 3]++;
    }
    for (uint32_t i = 0; i < 256 - 1; i++) {
        // pcounts[i + 1] += pcounts[i];
        counts[i * 4 + 4] += counts[i * 4];
        counts[i * 4 + 5] += counts[i * 4 + 1];
        counts[i * 4 + 6] += counts[i * 4 + 2];
        counts[i * 4 + 7] += counts[i * 4 + 3];
    }
    // sorting pass
    for (uint32_t i = length; i--;) {
        uint32_t v = src[i];
        uint32_t j = ((v << 2) & 0x3fc) + 0;
        buffer[--counts[j]] = v;
    }
    for (uint32_t i = length; i--;) {
        uint32_t v = buffer[i];
        uint32_t j = ((v >> 6) & 0x3fc) + 1;
        dest[--counts[j]] = v;
    }
    for (uint32_t i = length; i--;) {
        uint32_t v = dest[i];
        uint32_t j = ((v >> 14) & 0x3fc) + 2;
        buffer[--counts[j]] = v;
    }
    for (uint32_t i = length; i--;) {
        uint32_t v = buffer[i];
        uint32_t j = ((v >> 22) & 0x3fc) + 3;
        dest[--counts[j]] = v;
    }
}