#pragma once

#include "types.h"
#include <stdint.h>

void sprintf_hash(char* str, const HashDigest hash);
void print_hash(const HashDigest hash);

uint64_t reverse_sha256(uint64_t start, uint64_t end, const HashDigest target);
uint64_t reverse_sha256_x4(
    uint64_t start,
    uint64_t end,
    const HashDigest target
);
uint64_t reverse_sha256_x4x2orx8(
    uint64_t start,
    uint64_t end,
    const HashDigest target
);

