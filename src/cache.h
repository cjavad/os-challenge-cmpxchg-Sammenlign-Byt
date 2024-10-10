#pragma once

#include "sha256/sha256.h"
#include "vec.h"
#include <assert.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#define CACHE_KEY_LENGTH SHA256_DIGEST_LENGTH
#define CACHE_KEY_HASH_MAX 256

typedef struct TreeNode TreeNode;

struct TreeNode {
    union {
        struct {
            uint32_t next[CACHE_KEY_HASH_MAX];
        } branch;

        struct {
            uint8_t data[CACHE_KEY_LENGTH];
            uint32_t length;
            uint32_t next;
        } edge;

        struct {
            uint64_t value;
        } leaf;
    };

    enum {
        TT_NONE = 0,
        TT_BRANCH,
        TT_EDGE,
        TT_LEAF,
    } type;
};

struct Cache {
    pthread_mutex_t lock;
    Vec(TreeNode) nodes;
};

typedef struct Cache Cache;

Cache* cache_create();
void cache_destroy(Cache* cache);

uint64_t cache_get(Cache* cache, HashDigest key);
void cache_insert(Cache* cache, HashDigest key, uint64_t value);

static void cache_debug_print_node(const Cache* cache, const TreeNode* node, const int indent) {
    switch (node->type) {
    case TT_BRANCH: {
        printf("%*sBranch\n", indent, "");
        for (int i = 0; i < CACHE_KEY_HASH_MAX; i++) {
            if (node->branch.next[i] != 0) {
                printf("%*s[%x]\n", indent + 2, "", i);
                cache_debug_print_node(cache, &vec_get(&cache->nodes, node->branch.next[i]), indent + 4);
            }
        }
    } break;

    case TT_EDGE: {
        printf("%*sEdge\n", indent, "");
        printf("%*sLength: %d\n", indent + 2, "", node->edge.length);
        printf("%*sData: ", indent + 2, "");
        for (int i = 0; i < node->edge.length; i++) {
            printf("%02x", node->edge.data[i]);
        }
        printf("\n");
        cache_debug_print_node(cache, &vec_get(&cache->nodes, node->edge.next), indent + 2);
    } break;

    case TT_LEAF: {
        printf("%*sLeaf\n", indent, "");
        printf("%*sValue: %lu\n", indent + 2, "", node->leaf.value);
    } break;

    case TT_NONE: {
        // Unreachable.
        printf("%*sNone\n", indent, "");
    }
    }
}

/**
*
* uint8_t cache_key_hash_hex(const HashDigest hash, const uint32_t idx) {
    return (hash[idx >> 1] & (0xf << (idx & 1))) >> (idx & 1);
*/

inline uint8_t cache_key_hash_idx(const HashDigest key, const uint64_t idx) {
    return key[idx];
}

static void cache_debug_print(const Cache* cache) {
    printf("===============================<CACHE>=================================\n");
    cache_debug_print_node(cache, &vec_get(&cache->nodes, 0), 0);
}