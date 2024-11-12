#include "sha256.h"
#include "../config.h"

#ifndef HASH_FUNC
#define HASH_FUNC sha256_fullyfused
#endif

#ifndef HASH_FUNC_X4
#define HASH_FUNC_X4 sha256x4_fullyfused
#endif

#define HASH_COMPARE_U64(ptr, target)                                          \
    ((ptr)[0] == (target)[0] && (ptr)[1] == (target)[1] && (ptr)[2] == (target)[2] &&      \
     (ptr)[3] == (target)[3])

uint64_t reverse_sha256(
    const uint64_t start,
    const uint64_t end,
    const HashDigest target
) {

    const uint64_t* target_u64 = (const uint64_t*)target;

    for (uint64_t i = start; i <= end; i++) {
        uint64_t out_64[4];

        HASH_FUNC((uint8_t*)out_64, (uint8_t*)&i);

        if (HASH_COMPARE_U64(out_64, target_u64)) {
            return i;
        }
    }

    return 0;
}

__attribute__((flatten))
uint64_t reverse_sha256_x4(
    const uint64_t start,
    const uint64_t end,
    const HashDigest target
) {
    const uint64_t* target_u64 = (const uint64_t*)target;
    uint64_t data[4] = {start, start + 1, start + 2, start + 3};

    do {
        uint64_t out_64[4 * 4];
        HASH_FUNC_X4((uint8_t*)out_64, (uint8_t*)data);

        // Smartly compare the results
        for (int i = 0; i < 4; i++) {
            if (HASH_COMPARE_U64((out_64 + (i * 4)), target_u64)) {
                return data[i];
            }
        }

        data[0] += 4;
        data[1] += 4;
        data[2] += 4;
        data[3] += 4;
    } while (data[0] <= end);

    return 0;
}