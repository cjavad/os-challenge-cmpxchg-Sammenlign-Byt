#pragma once

#include "sha256/sha256.h"
#include "vec.h"
#include <assert.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#define CACHE_KEY_LENGTH_PACKED (SHA256_DIGEST_LENGTH)
#define CACHE_KEY_LENGTH (SHA256_DIGEST_LENGTH * 2)
#define CACHE_KEY_HASH_MAX 16

typedef uint8_t cache_key_t;
typedef uint8_t cache_key_idx_t;

enum TreeNodeType
{
    TT_NONE = 0,
    TT_BRANCH = 1,
    TT_EDGE = 2,
    TT_LEAF = 3,
};

struct TreeNodePointer
{
    uint32_t type : 2;
    uint32_t idx : 30;
};

typedef struct TreeNodePointer TreeNodePointer;

struct TreeNodeBranch
{
    TreeNodePointer next[CACHE_KEY_HASH_MAX];
};

struct TreeNodeEdge
{
    union
    {
        cache_key_t data[4];
        uint32_t str_idx;
    };

    cache_key_idx_t length;
    TreeNodePointer next;
};

struct TreeNodeLeaf
{
    uint64_t value;
};

union TreeNodeKeyEntry
{
    cache_key_t str[CACHE_KEY_LENGTH];
};

struct Cache
{
    pthread_mutex_t lock;
    TreeNodePointer root;

    FreeList(union TreeNodeKeyEntry) strings;

    FreeList(struct TreeNodeBranch) branches;

    FreeList(struct TreeNodeEdge) edges;

    FreeList(struct TreeNodeLeaf) leaves;
};

typedef struct Cache Cache;


#define CACHE_LEAF(cache, node) (&(cache)->leaves.data[(node).idx])
#define CACHE_EDGE(cache, node) (&(cache)->edges.data[(node).idx])
#define CACHE_BRANCH(cache, node) (&(cache)->branches.data[(node).idx])


#define CACHE_EDGE_FETCH_STR(variable, cache, edge) (variable) = ((edge)->length > 4 ? (cache)->strings.data[(edge)->str_idx].str : (edge)->data)

#define CACHE_LEAF_REFETCH(variable, cache, node) (variable) = CACHE_LEAF(cache, node)
#define CACHE_EDGE_REFETCH(variable, cache, node) (variable) = CACHE_EDGE(cache, node)
#define CACHE_BRANCH_REFETCH(variable, cache, node) (variable) = CACHE_BRANCH(cache, node)

Cache* cache_create(uint32_t default_cap);
void cache_destroy(Cache* cache);

uint64_t cache_get(Cache* cache, HashDigest key);
void cache_insert(Cache* cache, HashDigest key, uint64_t value);

static void cache_debug_print_node(const Cache* cache, const TreeNodePointer* node, const int indent)
{
    switch (node->type)
    {
    case TT_BRANCH:
        {
            const struct TreeNodeBranch* branch = CACHE_BRANCH(cache, *node);
            printf("%*sBranch\n", indent, "");
            for (int i = 0; i < CACHE_KEY_HASH_MAX; i++)
            {
                if (branch->next[i].type != TT_NONE)
                {
                    printf("%*s[%x]\n", indent + 2, "", i);
                    cache_debug_print_node(cache, &branch->next[i], indent + 4);
                }
            }
        }
        break;

    case TT_EDGE:
        {
            const struct TreeNodeEdge* edge = CACHE_EDGE(cache, *node);

            printf("%*sEdge\n", indent, "");
            printf("%*sLength: %d\n", indent + 2, "", edge->length);
            printf("%*sData: ", indent + 2, "");
            const cache_key_t* edge_str;
            CACHE_EDGE_FETCH_STR(edge_str, cache, edge);

            for (int i = 0; i < edge->length; i++)
            {
                printf("%01x", edge_str[i]);
            }
            printf("\n");

            if (edge->next.type == TT_NONE)
            {
                return;
            }

            cache_debug_print_node(cache, &edge->next, indent + 2);
        }
        break;

    case TT_LEAF:
        {
            const struct TreeNodeLeaf* leaf = CACHE_LEAF(cache, *node);
            printf("%*sLeaf\n", indent, "");
            printf("%*sValue: %lu\n", indent + 2, "", leaf->value);
        }
        break;

    case TT_NONE:
        {
            printf("%*sNone\n", indent, "");
        }
    }
}

// /*
inline cache_key_t cache_key_hash(const HashDigest hash, const uint32_t idx)
{
    return (hash[idx >> 1] >> ((~idx & 1) << 2)) & 0xf;
}

//*/

/*
inline cache_key_t cache_key_hash_idx(const HashDigest key, const uint64_t idx) {
    return key[idx];
}
//*/

static void cache_debug_print(const Cache* cache)
{
    printf("===============================<CACHE>=================================\n");
    cache_debug_print_node(cache, &cache->root, 0);
}

static void cache_debug_print_idx(const HashDigest hash)
{
    printf("\t");
    for (uint32_t i = 0; i < 32; i++)
    {
        printf("%02x ", hash[i]);
    }
    printf("\n\t");
    for (uint32_t i = 0; i < 32; i++)
    {
        printf("%01x%01x ", cache_key_hash(hash, i * 2), cache_key_hash(hash, i * 2 + 1));
    }
    printf("\n");
};
