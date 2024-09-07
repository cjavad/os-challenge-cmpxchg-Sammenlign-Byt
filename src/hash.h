#pragma once

#include <stdint.h>
#include <stdio.h>

typedef union {
    uint64_t u64[4];
    uint32_t u32[8];
    uint8_t u8[32];
} hash_u;

void debug_print_hash(hash_u hash);

int sha256_hash(const uint8_t *data, size_t len, hash_u* hash);

uint64_t reverse_hash(uint64_t start, uint64_t end, hash_u target);