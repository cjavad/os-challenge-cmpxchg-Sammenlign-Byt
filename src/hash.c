#include "hash.h"
#include "sha256/sha256.h"

uint64_t reverse_hash(uint64_t start, uint64_t end, HashDigest target) {

    for (uint64_t i = start; i < end; i++) {
        HashDigest out;

        sha256_fused(out, (uint8_t *)&i);

        if (out[0] == target[0] && __builtin_memcmp(out, target, 32) == 0) {
            return i;
        }
    }

    return 0;
}