#include "hash.h"

uint64_t reverse_hash(uint64_t start, uint64_t end, HashDigest target) {
    for (uint64_t i = start; i < end; i++) {
	HashDigest out;

	sha256_fullyfused(out, (uint8_t*)&i);

	if (out[0] == target[0] &&
	    memcmp(out, target, SHA256_DIGEST_LENGTH) == 0) {
	    return i;
	}
    }

    return 0;
}

uint64_t reverse_hash_x4(uint64_t start, uint64_t end, HashDigest target) {
    uint64_t data[4] = {start, start + 1, start + 2, start + 3};
    uint8_t hash[SHA256_DIGEST_LENGTH * 4];

    do {
	sha256x4_fullyfused(hash, (uint8_t*)data);

	// Smartly compare the results
	for (int i = 0; i < 4; i++) {
	    if (hash[i * SHA256_DIGEST_LENGTH] == target[0] &&
	        memcmp(
	            &hash[i * SHA256_DIGEST_LENGTH], target,
	            SHA256_DIGEST_LENGTH
	        ) == 0) {
		return data[i];
	    }
	}

	data[0] += 4;
	data[1] += 4;
	data[2] += 4;
	data[3] += 4;
    } while (data[0] < end);

    return 0;
}
