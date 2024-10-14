#pragma once

#include "radix_tree.h"
#include "ringbuffer.h"
#include "sha256/sha256.h"
#include <assert.h>
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>

struct PendingCacheEntry {
    HashDigest key;
    uint64_t value;
};

struct Cache {
    RingBuffer(struct PendingCacheEntry) pending;
    RadixTree(uint64_t, SHA256_DIGEST_LENGTH) tree;
};

typedef struct Cache Cache;

Cache* cache_create(uint32_t default_cap);
void cache_destroy(Cache* cache);

void cache_process_pending(Cache* cache);
void cache_insert_pending(Cache* cache, const HashDigest key, uint64_t value);