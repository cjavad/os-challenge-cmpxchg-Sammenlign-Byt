//
// Created by javad on 10-10-24.
//

#include "cache.h"
#include <string.h>

Cache* cache_create(const uint32_t default_cap) {
    Cache* cache = calloc(1, sizeof(Cache));

    ringbuffer_init(&cache->pending, 1024);
    radix_tree_create(&cache->tree, default_cap);

    return cache;
}

void cache_destroy(Cache* cache) {
    ringbuffer_destroy(&cache->pending);
    radix_tree_destroy(&cache->tree);
    free(cache);
}

void cache_process_pending(Cache* cache) {
    // Only process up to the current tail at the time of the call.
    struct PendingCacheEntry entry = {0};
    const uint32_t head = ringbuffer_head(&cache->pending);
    uint32_t tail = ringbuffer_tail(&cache->pending);

    while (head != tail) {
        ringbuffer_pop(&cache->pending, &entry);
        tail = ringbuffer_tail(&cache->pending);
        radix_tree_insert(&cache->tree, entry.key, SHA256_DIGEST_LENGTH, &entry.value);
    }
}

void cache_insert_pending(Cache* cache, const HashDigest key, const uint64_t value) {
    const struct PendingCacheEntry entry = {.value = value};
    memcpy(entry.key, key, sizeof(HashDigest));
    ringbuffer_push(&cache->pending, entry);
}