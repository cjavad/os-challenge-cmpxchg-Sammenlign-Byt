#pragma once

#include "freelist.h"

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
__attribute__((always_inline)) inline radix_key_t radix_tree_key_unpack(
    const radix_key_t* key, const radix_key_idx_t idx
) {
    return (key[idx >> 1] >> ((~idx & 1) << 2)) & 0xf;
}
/// Pack two 0-15 (4 bit) values into a 0-255 (8 bit) value
__attribute__((always_inline)) inline void radix_tree_key_pack(
    radix_key_t* key, const radix_key_idx_t idx, const radix_key_t value
) {
    key[idx >> 1] = (key[idx >> 1] & (0xf << ((idx & 1) << 2))) | (value << ((~idx & 1) << 2));
}

/// Copy a packed key respecting overlapping 4-bit chunks.
__attribute__((always_inline)) inline void radix_tree_copy_key(
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
    RTT_BRANCH,
    RTT_EDGE,
    RTT_LEAF,
};

struct RadixTreeNodePtr {
    uint32_t type : 2;
    uint32_t idx : 30;
};

struct RadixTreeBranchNode {
    struct RadixTreeNodePtr next[RADIX_TREE_KEY_BRANCHES + 1];
};

struct RadixTreeEdgeNode {
    struct RadixTreeNodePtr next;

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
};

#if RADIX_TREE_MALLOC_STRS
#define RadixTree(value_type, key_length)                                                                              \
    struct {                                                                                                           \
        struct RadixTreeNodePtr root;                                                                                  \
        FreeList(struct RadixTreeBranchNode) branches;                                                                 \
        FreeList(struct RadixTreeEdgeNode) edges;                                                                      \
        FreeList(value_type) leaves;                                                                                   \
    }
#else
#define RadixTree(value_type, key_length)                                                                              \
    struct {                                                                                                           \
        struct RadixTreeNodePtr root;                                                                                  \
        FreeList(struct RadixTreeBranchNode) branches;                                                                 \
        FreeList(struct RadixTreeEdgeNode) edges;                                                                      \
        FreeList(value_type) leaves;                                                                                   \
        FreeList(radix_key_t) strings;                                                                                 \
        struct {                                                                                                       \
            char _[key_length];                                                                                        \
        } kl[0];                                                                                                       \
    }
#endif

typedef RadixTree(void*, 0) _RadixTreeBase;

struct RadixTreeNodePtr radix_tree_create_branch_node(_RadixTreeBase* tree);

struct RadixTreeNodePtr radix_tree_create_leaf_node(_RadixTreeBase* tree, const void* value_ptr, uint32_t value_size);

struct RadixTreeNodePtr radix_tree_create_edge_node(
    _RadixTreeBase* tree, const radix_key_t* key, radix_key_idx_t key_size, radix_key_idx_t offset,
    radix_key_idx_t key_length, struct RadixTreeNodePtr next
);

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
        (tree)->root.type = RTT_NONE;                                                                                  \
        freelist_init(&(tree)->branches, (default_cap + 16) / 16);                                                     \
        freelist_init(&(tree)->edges, default_cap);                                                                    \
        freelist_init(&(tree)->leaves, default_cap);                                                                   \
    }
#else
#define radix_tree_create(tree, default_cap)                                                                           \
    {                                                                                                                  \
        (tree)->root.type = RTT_NONE;                                                                                  \
        freelist_init(&(tree)->branches, (default_cap + 16) / 16);                                                     \
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
        freelist_destroy(&(tree)->branches);                                                                           \
        freelist_destroy(&(tree)->edges);                                                                              \
        freelist_destroy(&(tree)->leaves);                                                                             \
    }
#else
#define radix_tree_destroy(tree)                                                                                       \
    {                                                                                                                  \
        freelist_destroy(&(tree)->strings);                                                                            \
        freelist_destroy(&(tree)->branches);                                                                           \
        freelist_destroy(&(tree)->edges);                                                                              \
        freelist_destroy(&(tree)->leaves);                                                                             \
    }
#endif

#define ____RT_TEMP(x, c) ____CONCAT(____CONCAT(____CONCAT(____rt_temp, x), ____), c)

#define radix_tree_set_prev(tree, prev, prev_branch_val, new_node)                                                     \
    {                                                                                                                  \
        switch ((prev).type) {                                                                                         \
        case RTT_EDGE: {                                                                                               \
            radix_tree_fetch_edge(tree, prev)->next = new_node;                                                        \
        } break;                                                                                                       \
        case RTT_BRANCH: {                                                                                             \
            radix_tree_fetch_branch(tree, prev)->next[prev_branch_val] = new_node;                                     \
        } break;                                                                                                       \
        case RTT_NONE: {                                                                                               \
            (tree)->root = new_node;                                                                                   \
        } break;                                                                                                       \
        default: {                                                                                                     \
            __builtin_unreachable();                                                                                   \
        }                                                                                                              \
        }                                                                                                              \
    }

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