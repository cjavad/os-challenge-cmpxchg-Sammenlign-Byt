#pragma once

#include "bits/radix_tree.h"
#include "bits/ringbuffer.h"
#include "sha256/types.h"
#include <assert.h>
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

struct PendingCacheEntry {
    HashDigest key;
    uint64_t value;
};

struct Cache {
    RingBuffer(struct PendingCacheEntry) pending;
    RadixTree(uint64_t, SHA256_DIGEST_LENGTH) tree;
};

struct Cache* cache_create(uint32_t default_cap);
void cache_destroy(struct Cache* cache);

void cache_process_pending(struct Cache* cache);
void cache_insert_pending(
    struct Cache* cache,
    const HashDigest key,
    uint64_t value
);