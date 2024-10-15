#pragma once

#include "freelist.h"

#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <unistd.h>

typedef uint8_t radix_key_t;
typedef uint8_t radix_key_idx_t;

#define RADIX_TREE_KEY_BRANCHES   16
#define RADIX_TREE_SMALL_STR_SIZE 14

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
    if (offset & 1 == 0) {
        memcpy(dest, src + (offset / 2), (length + 1) / 2);
        return;
    }

    for (radix_key_idx_t i = 0; i < length; i++) {
        radix_tree_key_pack(dest, i, radix_tree_key_unpack(src, offset + i));
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
    struct RadixTreeNodePtr next[RADIX_TREE_KEY_BRANCHES];
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
            radix_key_t data[7];
        } __attribute__((packed));
        struct {
            uint32_t string_idx;
            uint8_t padding[3];
        } __attribute__((packed));
    };

    radix_key_idx_t length;
};

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

typedef RadixTree(void*, 0) _RadixTreeBase;

void _radix_tree_insert(
    _RadixTreeBase* tree, const radix_key_t* new_key, radix_key_idx_t key_length, const void* value, uint32_t value_size
);

void _radix_tree_get(
    const _RadixTreeBase* tree, const radix_key_t* key, radix_key_idx_t key_length, void* value, uint32_t value_size
);

void radix_tree_debug_node(
    _RadixTreeBase* tree, struct RadixTreeNodePtr node, FILE* stream, uint32_t indent, uint32_t key_length,
    uint32_t value_size
);

#define radix_tree_fetch_branch(tree, ptr)            (&(tree)->branches.data[(ptr).idx])
#define radix_tree_fetch_leaf_unsafe(tree, ptr, size) freelist_get_unsafe(&(tree)->leaves, (ptr).idx, (size))

#define radix_tree_fetch_edge(tree, ptr) (&(tree)->edges.data[(ptr).idx])

#define radix_tree_fetch_edge_str_unsafe(tree, edge, key_length)                                                       \
    ((edge)->length > RADIX_TREE_SMALL_STR_SIZE                                                                        \
         ? (freelist_get_unsafe(&(tree)->strings, (edge)->string_idx, (key_length)))                                   \
         : (radix_key_t*)(edge)->data)

#define radix_tree_refetch_edge(variable, tree, ptr)                                                                   \
    { (variable) = radix_tree_fetch_edge(tree, ptr); }
#define radix_tree_refetch_edge_str_unsafe(variable, tree, edge, key_length)                                           \
    { (variable) = radix_tree_fetch_edge_str_unsafe(tree, edge, key_length); }

#define radix_tree_key_length(tree) (sizeof((tree)->kl[0]._))
#define radix_tree_value_type(tree) typeof((tree)->leaves.data[0])
#define radix_tree_value_size(tree) (sizeof((tree)->leaves.data[0]))

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

#define radix_tree_destroy(tree)                                                                                       \
    {                                                                                                                  \
        freelist_destroy(&(tree)->strings);                                                                            \
        freelist_destroy(&(tree)->branches);                                                                           \
        freelist_destroy(&(tree)->edges);                                                                              \
        freelist_destroy(&(tree)->leaves);                                                                             \
    }

#define ____RT_TEMP(x, c) ____CONCAT(____CONCAT(____CONCAT(____rt_temp, x), ____), c)

#define ____radix_tree_create_leaf_node(tree, value_ptr, value_size, c)                                                \
    ({                                                                                                                 \
        const __auto_type ____RT_TEMP(tmp, c) = (value_ptr);                                                           \
        const uint32_t idx = freelist_insert_unsafe(&(tree)->leaves, (____RT_TEMP(tmp, c)), (value_size));             \
        (struct RadixTreeNodePtr){.type = RTT_LEAF, .idx = idx};                                                       \
    })

#define radix_tree_create_leaf_node(tree, value_ptr, value_size)                                                       \
    ____radix_tree_create_leaf_node(tree, value_ptr, value_size, __COUNTER__)

#define ____radix_tree_create_branch_node(tree, c)                                                                     \
    ({                                                                                                                 \
        const __auto_type ____RT_TEMP(tmp, c) = ((const struct RadixTreeBranchNode){0});                               \
        const uint32_t idx = freelist_insert(&(tree)->branches, (____RT_TEMP(tmp, c)));                                \
        (struct RadixTreeNodePtr){.type = RTT_BRANCH, .idx = idx};                                                     \
    })

#define radix_tree_create_branch_node(tree) ____radix_tree_create_branch_node(tree, __COUNTER__)

#define ____radix_tree_create_edge_node(tree, key, key_length, offset, cur_key_length, next_node, c)                   \
    ({                                                                                                                 \
        const __auto_type ____RT_TEMP(edge, c) =                                                                       \
            ((struct RadixTreeEdgeNode){.length = (cur_key_length), .next = (next_node)});                             \
        const uint32_t ____RT_TEMP(idx, c) = freelist_insert(&(tree)->edges, (____RT_TEMP(edge, c)));                  \
        if (____RT_TEMP(edge, c).length > RADIX_TREE_SMALL_STR_SIZE) {                                                 \
            radix_key_t ____RT_TEMP(buf, c)[key_length];                                                               \
            radix_tree_copy_key(____RT_TEMP(buf, c), key, offset, ____RT_TEMP(edge, c).length);                        \
            (tree)->edges.data[____RT_TEMP(idx, c)].string_idx =                                                       \
                freelist_insert_unsafe(&(tree)->strings, ____RT_TEMP(buf, c), (key_length));                           \
        } else {                                                                                                       \
            radix_tree_copy_key(                                                                                       \
                (tree)->edges.data[____RT_TEMP(idx, c)].data, key, offset, ____RT_TEMP(edge, c).length                 \
            );                                                                                                         \
        }                                                                                                              \
        (struct RadixTreeNodePtr){.type = RTT_EDGE, .idx = ____RT_TEMP(idx, c)};                                       \
    })

#define radix_tree_create_edge_node(tree, key, key_length, offset, length, next)                                       \
    ____radix_tree_create_edge_node(tree, key, key_length, offset, length, next, __COUNTER__)

#define radix_tree_insert(tree, key, value)                                                                            \
    _radix_tree_insert((_RadixTreeBase*)tree, key, radix_tree_key_length(tree), value, radix_tree_value_size(tree))

#define ____radix_tree_get(tree, key, c)                                                                               \
    ({                                                                                                                 \
        radix_tree_value_type(tree) ____RT_TEMP(val, c);                                                               \
        _radix_tree_get(                                                                                               \
            (_RadixTreeBase*)tree, key, radix_tree_key_length(tree), &____RT_TEMP(val, c), radix_tree_value_size(tree) \
        );                                                                                                             \
        ____RT_TEMP(val, c);                                                                                           \
    })

#define radix_tree_get(tree, key) ____radix_tree_get(tree, key, __COUNTER__)

#define radix_tree_debug(tree, stream)                                                                                 \
    radix_tree_debug_node(                                                                                             \
        (_RadixTreeBase*)tree, (tree)->root, stream, 0, radix_tree_key_length(tree),                                   \
        sizeof(radix_tree_value_type(tree))                                                                            \
    )