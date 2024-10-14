#pragma once

#include "radix_tree.h"
#include "ringbuffer.h"
#include "sha256/sha256.h"
#include <assert.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#define CACHE_KEY_LENGTH_PACKED (SHA256_DIGEST_LENGTH)
#define CACHE_KEY_LENGTH        (SHA256_DIGEST_LENGTH * 2)

struct PendingCacheEntry {
    radix_key_t key[CACHE_KEY_LENGTH_PACKED];
    uint64_t value;
};

struct Cache {
    RingBuffer(struct PendingCacheEntry) pending;
    RadixTree(uint64_t, CACHE_KEY_LENGTH) tree;
};

typedef struct Cache Cache;

Cache* cache_create(uint32_t default_cap);
void cache_destroy(Cache* cache);

void cache_process_pending(Cache* cache);
void cache_insert_pending(Cache* cache, const HashDigest key, uint64_t value);

/*static void cache_debug_print_node(const Cache* cache, const TreeNodePointer* node, const int indent) {
    switch (node->type) {
    case TT_BRANCH: {
        const struct TreeNodeBranch* branch = CACHE_BRANCH(cache, *node);
        printf("%*sBranch\n", indent, "");
        for (int i = 0; i < CACHE_KEY_HASH_MAX; i++) {
            if (branch->next[i].type != TT_NONE) {
                printf("%*s[%x]\n", indent + 2, "", i);
                cache_debug_print_node(cache, &branch->next[i], indent + 4);
            }
        }
    } break;

    case TT_EDGE: {
        const struct TreeNodeEdge* edge = CACHE_EDGE(cache, *node);

        printf("%*sEdge\n", indent, "");
        printf("%*sLength: %d\n", indent + 2, "", edge->length);
        printf("%*sData: ", indent + 2, "");
        const cache_key_t* edge_str;
        CACHE_EDGE_FETCH_STR(edge_str, cache, edge);

        for (int i = 0; i < edge->length; i++) {
            printf("%01x", cache_key_hash_get(edge_str, i));
        }
        printf("\n");

        if (edge->next.type == TT_NONE) {
            return;
        }

        cache_debug_print_node(cache, &edge->next, indent + 2);
    } break;

    case TT_LEAF: {
        const struct TreeNodeLeaf* leaf = CACHE_LEAF(cache, *node);
        printf("%*sLeaf\n", indent, "");
        printf("%*sValue: %lu\n", indent + 2, "", leaf->value);
    } break;

    case TT_NONE: {
        printf("%*sNone\n", indent, "");
    }
    }
}

static void cache_debug_print(const Cache* cache) {
    printf("===============================<CACHE>=================================\n");
    cache_debug_print_node(cache, &cache->root, 0);
}

static void cache_debug_print_idx(const HashDigest hash) {
    printf("\t");
    for (uint32_t i = 0; i < 32; i++) {
        printf("%02x ", hash[i]);
    }
    printf("\n\t");
    for (uint32_t i = 0; i < 32; i++) {
        printf("%01x%01x ", cache_key_hash_get(hash, i * 2), cache_key_hash_get(hash, i * 2 + 1));
    }
    printf("\n");
};*/
