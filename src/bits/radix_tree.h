#pragma once

#include "freelist.h"
#include "vec.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <unistd.h>

typedef uint8_t radix_key_t;
typedef uint16_t radix_key_idx_t;

#define RADIX_TREE_KEY_BITS     4
#define RADIX_TREE_KEY_RATIO    2
#define RADIX_TREE_KEY_BRANCHES 16
#define RADIX_TREE_MALLOC_STRS  0
// Allow us to change the radix_key_idx_t size to gain more string space in the edge node.
#if RADIX_TREE_MALLOC_STRS
#define RADIX_TREE_EDGE_PADDING   (12 - sizeof(radix_key_t*) - sizeof(radix_key_idx_t))
#define RADIX_TREE_EDGE_DATA_SIZE (sizeof(radix_key_t*) + RADIX_TREE_EDGE_PADDING)
#else
#define RADIX_TREE_EDGE_PADDING   (4 - sizeof(radix_key_idx_t))
#define RADIX_TREE_EDGE_DATA_SIZE (4 + RADIX_TREE_EDGE_PADDING)
#endif

#define RADIX_TREE_SMALL_STR_SIZE (RADIX_TREE_EDGE_DATA_SIZE * RADIX_TREE_KEY_RATIO)

// 16 branches + 1 immediate
#define RADIX_BRANCH_IMMEDIATE RADIX_TREE_KEY_BRANCHES

/// Unpack a 0-255 (8 bits) value into two 0-15 (4 bit) values
/// We do this by indexing twice the length of the key
inline radix_key_t radix_tree_key_unpack(const radix_key_t* key, const radix_key_idx_t idx) {
    return (key[idx >> 1] >> ((~idx & 1) << 2)) & 0xf;
}
/// Pack two 0-15 (4 bit) values into a 0-255 (8 bit) value
inline void radix_tree_key_pack(radix_key_t* key, const radix_key_idx_t idx, const radix_key_t value) {
    key[idx >> 1] = (key[idx >> 1] & (0xf << ((idx & 1) << 2))) | (value << ((~idx & 1) << 2));
}

/// Copy a packed key respecting overlapping 4-bit chunks.
inline void radix_tree_copy_key(
    radix_key_t* dest, const radix_key_t* src, const radix_key_idx_t offset, const radix_key_idx_t length
) {
    if ((offset & 1) == 0) {
        memcpy(dest, src + (offset / RADIX_TREE_KEY_RATIO), (length + 1) / RADIX_TREE_KEY_RATIO);
        return;
    }

    for (radix_key_idx_t i = 0; i < length; i++) {
        const radix_key_t value = radix_tree_key_unpack(src, offset + i);
        radix_tree_key_pack(dest, i, value);
    }
}

enum RadixTreeNodeType {
    RTT_NONE = 0,
    RTT_EDGE,
    RTT_LEAF,
    RTT_BRANCH_4,
    RTT_BRANCH_8,
    RTT_BRANCH_16,
    RTT_BRANCH_FULL,
};

struct RadixTreeNodePtr {
    uint32_t type : 3;
    uint32_t idx : 29;
};

// Fits in cache line + fast lookup
struct RadixTreeBranch4Node {
    radix_key_t values[4];
    struct RadixTreeNodePtr entries[4];
};

// Fits in cache line + slow lookup
struct RadixTreeBranch8Node {
    radix_key_t values[8];
    struct RadixTreeNodePtr entries[8];
};

// Fits in cache line + fast lookup
struct RadixTreeBranch16Node {
    struct RadixTreeNodePtr entries[RADIX_TREE_KEY_BRANCHES];
};

// Does not fit in cache line + fast lookup
struct RadixTreeBranchFullNode {
    struct RadixTreeNodePtr entries[RADIX_TREE_KEY_BRANCHES + 1];
};

struct RadixTreeEdgeNode {
    // Take advantage of the fact that the edge
    // node struct will always be 4-byte aligned
    // so we have an extra 3 bytes of data we can
    // use (7 bytes in total) before we need to
    // use the string buffer. This is typically not
    // important for our use case but for general keys
    // it is very nice.
    union {
        struct {
            radix_key_t data[RADIX_TREE_EDGE_DATA_SIZE];
        } __attribute__((packed));
        struct {
#if RADIX_TREE_MALLOC_STRS
            radix_key_t* string_ptr;
#else
            uint32_t string_idx;
#endif
            uint8_t padding[RADIX_TREE_EDGE_PADDING];
        } __attribute__((packed));
    };

    radix_key_idx_t length;
    struct RadixTreeNodePtr next;
};
struct RadixTreePrevPtr {
    struct RadixTreeNodePtr node;
    radix_key_t branch_val;
};

#if RADIX_TREE_MALLOC_STRS
#define RadixTree(value_type, key_length)                                                                              \
    struct {                                                                                                           \
        struct RadixTreeNodePtr root;                                                                                  \
        Vec(struct RadixTreePrevPtr) prev;                                                                             \
        FreeList(struct RadixTreeBranch4Node) branches4;                                                               \
        FreeList(struct RadixTreeBranch8Node) branches8;                                                               \
        FreeList(struct RadixTreeBranch16Node) branches16;                                                             \
        FreeList(struct RadixTreeBranchFullNode) branches_full;                                                        \
        FreeList(struct RadixTreeEdgeNode) edges;                                                                      \
        FreeList(value_type) leaves;                                                                                   \
    }
#else
#define RadixTree(value_type, key_length)                                                                              \
    struct {                                                                                                           \
        struct RadixTreeNodePtr root;                                                                                  \
        Vec(struct RadixTreePrevPtr) prev;                                                                             \
        FreeList(struct RadixTreeBranch4Node) branches4;                                                               \
        FreeList(struct RadixTreeBranch8Node) branches8;                                                               \
        FreeList(struct RadixTreeBranch16Node) branches16;                                                             \
        FreeList(struct RadixTreeBranchFullNode) branches_full;                                                        \
        FreeList(struct RadixTreeEdgeNode) edges;                                                                      \
        FreeList(value_type) leaves;                                                                                   \
        FreeList(radix_key_t) strings;                                                                                 \
        struct {                                                                                                       \
            char _[key_length];                                                                                        \
        } kl[0];                                                                                                       \
    }
#endif

typedef RadixTree(void*, 0) _RadixTreeBase;

struct RadixTreeNodePtr radix_tree_create_branch_node(_RadixTreeBase* tree, enum RadixTreeNodeType branch_type);

struct RadixTreeNodePtr radix_tree_create_leaf_node(_RadixTreeBase* tree, const void* value_ptr, uint32_t value_size);

struct RadixTreeNodePtr radix_tree_create_edge_node(
    _RadixTreeBase* tree, const radix_key_t* key, radix_key_idx_t key_size, radix_key_idx_t offset,
    radix_key_idx_t key_length, struct RadixTreeNodePtr next
);

void radix_tree_get_branch(
    const _RadixTreeBase* tree, const struct RadixTreeNodePtr* node, radix_key_t key, struct RadixTreeNodePtr* out
);
void radix_tree_get_branch4(const struct RadixTreeBranch4Node* branch, radix_key_t key, struct RadixTreeNodePtr* node);
void radix_tree_get_branch8(const struct RadixTreeBranch8Node* branch, radix_key_t key, struct RadixTreeNodePtr* node);
void radix_tree_get_branch16(
    const struct RadixTreeBranch16Node* branch, radix_key_t key, struct RadixTreeNodePtr* node
);
void radix_tree_get_branch_full(
    const struct RadixTreeBranchFullNode* branch, radix_key_t key, struct RadixTreeNodePtr* node
);

void radix_tree_set_prev(_RadixTreeBase* tree, struct RadixTreeNodePtr new_node);

struct RadixTreeNodePtr radix_tree_insert_into_branch_or_grow(
    _RadixTreeBase* tree, struct RadixTreeNodePtr branch_node, radix_key_t key, struct RadixTreeNodePtr node
);

struct RadixTreeNodePtr radix_tree_insert_into_branch4_or_grow(
    _RadixTreeBase* tree, struct RadixTreeBranch4Node* branch, radix_key_t key, struct RadixTreeNodePtr node
);

struct RadixTreeNodePtr radix_tree_insert_into_branch8_or_grow(
    _RadixTreeBase* tree, struct RadixTreeBranch8Node* branch, radix_key_t key, struct RadixTreeNodePtr node
);

struct RadixTreeNodePtr radix_tree_insert_into_branch16_or_grow(
    _RadixTreeBase* tree, struct RadixTreeBranch16Node* branch, radix_key_t key, struct RadixTreeNodePtr node
);

void radix_tree_insert_into_branch_full(
    struct RadixTreeBranchFullNode* branch, radix_key_t key, struct RadixTreeNodePtr node
);

void radix_tree_push_prev(_RadixTreeBase* tree, struct RadixTreeNodePtr node, radix_key_idx_t branch_val);

void _radix_tree_insert(
    _RadixTreeBase* tree, const radix_key_t* new_key, radix_key_idx_t key_len, radix_key_idx_t key_size,
    const void* value, uint32_t value_size
);

void _radix_tree_get(
    const _RadixTreeBase* tree, const radix_key_t* key, radix_key_idx_t key_len, radix_key_idx_t key_size, void** value,
    uint32_t value_size
);

void radix_tree_debug_node(
    _RadixTreeBase* tree, struct RadixTreeNodePtr node, FILE* stream, uint32_t indent, uint32_t key_size,
    uint32_t value_size
);

#define radix_tree_fetch_leaf_unsafe(tree, ptr, size) freelist_get_unsafe(&(tree)->leaves, (ptr).idx, (size))

#define radix_tree_fetch_branch(tree, ptr) (&(tree)->branches.data[(ptr).idx])

#define radix_tree_fetch_branch4(tree, ptr)     (&(tree)->branches4.data[(ptr).idx])
#define radix_tree_fetch_branch8(tree, ptr)     (&(tree)->branches8.data[(ptr).idx])
#define radix_tree_fetch_branch16(tree, ptr)    (&(tree)->branches16.data[(ptr).idx])
#define radix_tree_fetch_branch_full(tree, ptr) (&(tree)->branches_full.data[(ptr).idx])

#define radix_tree_refetch_branch(variable, tree, ptr)                                                                 \
    { (variable) = radix_tree_fetch_branch(tree, ptr); }

#define radix_tree_fetch_edge(tree, ptr) (&(tree)->edges.data[(ptr).idx])
#define radix_tree_refetch_edge(variable, tree, ptr)                                                                   \
    { (variable) = radix_tree_fetch_edge(tree, ptr); }

#if RADIX_TREE_MALLOC_STRS
#define radix_tree_fetch_edge_str_unsafe(tree, edge, key_size)                                                         \
    ((edge)->length > RADIX_TREE_SMALL_STR_SIZE ? (edge)->string_ptr : (radix_key_t*)(edge)->data)
#define radix_tree_refetch_edge_str_unsafe

#else
#define radix_tree_fetch_edge_str_unsafe(tree, edge, key_size)                                                         \
    ((edge)->length > RADIX_TREE_SMALL_STR_SIZE                                                                        \
         ? (freelist_get_unsafe(&(tree)->strings, (edge)->string_idx, (key_size)))                                     \
         : (radix_key_t*)(edge)->data)

#define radix_tree_refetch_edge_str_unsafe(variable, tree, edge, key_size)                                             \
    { (variable) = radix_tree_fetch_edge_str_unsafe(tree, edge, key_size); }
#endif

#if RADIX_TREE_MALLOC_STRS
#define radix_tree_key_length(tree) (0)
#else
#define radix_tree_key_length(tree) (sizeof((tree)->kl[0]._))
#endif

#define radix_tree_value_type(tree) typeof((tree)->leaves.data[0])
#define radix_tree_value_size(tree) (sizeof((tree)->leaves.data[0]))

#if RADIX_TREE_MALLOC_STRS
#define radix_tree_create(tree, default_cap)                                                                           \
    {                                                                                                                  \
        (tree)->root = (struct RadixTreeNodePtr){.type = RTT_NONE};                                                    \
        vec_init(&(tree)->prev, 64);                                                                                   \
        freelist_init(&(tree)->branches4, (default_cap + 32) / 32);                                                    \
        freelist_init(&(tree)->branches8, (default_cap + 32) / 32);                                                    \
        freelist_init(&(tree)->branches16, (default_cap + 32) / 32);                                                   \
        freelist_init(&(tree)->branches_full, (default_cap + 32) / 32);                                                \
        freelist_init(&(tree)->edges, default_cap);                                                                    \
        freelist_init(&(tree)->leaves, default_cap);                                                                   \
    }
#else
#define radix_tree_create(tree, default_cap)                                                                           \
    {                                                                                                                  \
        (tree)->root = (struct RadixTreeNodePtr){.type = RTT_NONE};                                                    \
        vec_init(&(tree)->prev, radix_tree_key_length(tree));                                                          \
        freelist_init(&(tree)->branches4, (default_cap + 32) / 32);                                                    \
        freelist_init(&(tree)->branches8, (default_cap + 32) / 32);                                                    \
        freelist_init(&(tree)->branches16, (default_cap + 32) / 32);                                                   \
        freelist_init(&(tree)->branches_full, (default_cap + 128) / 128);                                                \
        freelist_init(&(tree)->edges, default_cap);                                                                    \
        freelist_init(&(tree)->leaves, default_cap);                                                                   \
        freelist_init_unsafe(                                                                                          \
            &(tree)->strings, radix_tree_key_length(tree) > RADIX_TREE_SMALL_STR_SIZE ? default_cap : 1,               \
            radix_tree_key_length(tree)                                                                                \
        );                                                                                                             \
    }
#endif

#if RADIX_TREE_MALLOC_STRS
#define radix_tree_destroy(tree)                                                                                       \
    {                                                                                                                  \
        vec_destroy(&(tree)->prev);                                                                                    \
        freelist_destroy(&(tree)->branches4);                                                                          \
        freelist_destroy(&(tree)->branches8);                                                                          \
        freelist_destroy(&(tree)->branches16);                                                                         \
        freelist_destroy(&(tree)->branches_full);                                                                      \
        freelist_destroy(&(tree)->edges);                                                                              \
        freelist_destroy(&(tree)->leaves);                                                                             \
    }
#else
#define radix_tree_destroy(tree)                                                                                       \
    {                                                                                                                  \
        vec_destroy(&(tree)->prev);                                                                                    \
        freelist_destroy(&(tree)->strings);                                                                            \
        freelist_destroy(&(tree)->branches4);                                                                          \
        freelist_destroy(&(tree)->branches8);                                                                          \
        freelist_destroy(&(tree)->branches16);                                                                         \
        freelist_destroy(&(tree)->branches_full);                                                                      \
        freelist_destroy(&(tree)->edges);                                                                              \
        freelist_destroy(&(tree)->leaves);                                                                             \
    }
#endif

#define ____RT_TEMP(x, c) ____CONCAT(____CONCAT(____CONCAT(____rt_temp, x), ____), c)

#if RADIX_TREE_MALLOC_STRS
#define ____radix_tree_edge_maybe_move_str(tree, edge, old_key, old_idx, c)                                            \
    {                                                                                                                  \
        if ((edge)->length > RADIX_TREE_SMALL_STR_SIZE && (old_idx) <= RADIX_TREE_SMALL_STR_SIZE) {                    \
            radix_key_t* ____RT_TEMP(old_str_ptr, c) = (edge)->string_ptr;                                             \
            memcpy((edge)->data, ____RT_TEMP(old_str_ptr, c), (old_idx) + 1 / RADIX_TREE_KEY_RATIO);                   \
            free(____RT_TEMP(old_str_ptr, c));                                                                         \
        }                                                                                                              \
    }
#else
#define ____radix_tree_edge_maybe_move_str(tree, edge, old_key, old_idx, c)                                            \
    {                                                                                                                  \
        if ((edge)->length > RADIX_TREE_SMALL_STR_SIZE && (old_idx) <= RADIX_TREE_SMALL_STR_SIZE) {                    \
            const uint32_t ____RT_TEMP(old_str_idx, c) = (edge)->string_idx;                                           \
            memcpy((edge)->data, old_key, (old_idx) + 1 / RADIX_TREE_KEY_RATIO);                                       \
            freelist_remove(&(tree)->strings, ____RT_TEMP(old_str_idx, c));                                            \
        }                                                                                                              \
    }
#endif

#define radix_tree_edge_maybe_move_str(tree, edge, old_key, old_idx)                                                   \
    ____radix_tree_edge_maybe_move_str(tree, edge, old_key, old_idx, __COUNTER__)

#if RADIX_TREE_MALLOC_STRS
#define radix_tree_edge_maybe_remove_str(tree, edge)                                                                   \
    {                                                                                                                  \
        if ((edge)->length > RADIX_TREE_SMALL_STR_SIZE) {                                                              \
            free((edge)->string_ptr);                                                                                  \
        }                                                                                                              \
    }
#else
#define radix_tree_edge_maybe_remove_str(tree, edge)                                                                   \
    {                                                                                                                  \
        if ((edge)->length > RADIX_TREE_SMALL_STR_SIZE) {                                                              \
            freelist_remove(&(tree)->strings, (edge)->string_idx);                                                     \
        }                                                                                                              \
    }
#endif

#define radix_tree_insert(tree, key, key_len, value)                                                                   \
    _radix_tree_insert(                                                                                                \
        (_RadixTreeBase*)tree, key, (key_len) * RADIX_TREE_KEY_RATIO, radix_tree_key_length(tree), value,              \
        radix_tree_value_size(tree)                                                                                    \
    )

#define radix_tree_get(tree, key, key_len, dptrval)                                                                    \
    _radix_tree_get(                                                                                                   \
        (_RadixTreeBase*)(tree), key, (key_len) * RADIX_TREE_KEY_RATIO, radix_tree_key_length(tree),                   \
        (void**)(dptrval), radix_tree_value_size(tree)                                                                 \
    );

#define radix_tree_debug(tree, stream)                                                                                 \
    radix_tree_debug_node(                                                                                             \
        (_RadixTreeBase*)tree, (tree)->root, stream, 0, radix_tree_key_length(tree),                                   \
        sizeof(radix_tree_value_type(tree))                                                                            \
    )
