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

typedef struct TreeNode TreeNode;


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
        uint8_t data[4];
        uint32_t str_idx;
    };

    uint8_t length;
    TreeNodePointer next;
};

struct TreeNodeLeaf
{
    uint64_t value;
};

union TreeNodeKeyEntry
{
    uint8_t str[CACHE_KEY_LENGTH];
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

Cache* cache_create(uint32_t default_cap);
void cache_destroy(Cache* cache);


uint64_t cache_get(Cache* cache, HashDigest key);
void cache_insert(Cache* cache, HashDigest key, uint64_t value);

static void cache_debug_print_node(const Cache* cache, const TreeNode* node, const int indent)
{
}

// /*
inline uint8_t cache_key_hash(const HashDigest hash, const uint32_t idx)
{
    return (hash[idx >> 1] >> ((~idx & 1) << 2)) & 0xf;
}

//*/

/*
inline uint8_t cache_key_hash_idx(const HashDigest key, const uint64_t idx) {
    return key[idx];
}
//*/

static void cache_debug_print(const Cache* cache)
{
    printf("===============================<CACHE>=================================\n");
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
