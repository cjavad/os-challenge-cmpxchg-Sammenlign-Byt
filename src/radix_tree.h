#pragma once

#include "freelist.h"

#include <assert.h>
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
    assert(length != 255);

    if (offset % 2 == 0) {
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
        FreeList(struct { radix_key_t string[key_length]; }) strings;                                                  \
        FreeList(struct RadixTreeBranchNode) branches;                                                                 \
        FreeList(struct RadixTreeEdgeNode) edges;                                                                      \
        FreeList(value_type) leaves;                                                                                   \
    }

#define radix_tree_fetch_branch(tree, ptr) (&(tree)->branches.data[(ptr).idx])
#define radix_tree_fetch_leaf(tree, ptr)   (&(tree)->leaves.data[(ptr).idx])
#define radix_tree_fetch_edge(tree, ptr)   (&(tree)->edges.data[(ptr).idx])
#define radix_tree_fetch_edge_str(tree, edge)                                                                          \
    ((edge)->length > RADIX_TREE_SMALL_STR_SIZE ? (tree)->strings.data[(edge)->string_idx].string                      \
                                                : (radix_key_t*)(edge)->data)

#define radix_tree_refetch_branch(variable, tree, ptr)                                                                 \
    { (variable) = radix_tree_fetch_branch(tree, ptr); }
#define radix_tree_refetch_leaf(variable, tree, ptr)                                                                   \
    { (variable) = radix_tree_fetch_leaf(tree, ptr); }
#define radix_tree_refetch_edge(variable, tree, ptr)                                                                   \
    { (variable) = radix_tree_fetch_edge(tree, ptr); }
#define radix_tree_refetch_edge_str(variable, tree, edge)                                                              \
    { (variable) = radix_tree_fetch_edge_str(tree, edge); }

#define radix_tree_key_length(tree) (sizeof((tree)->strings.data[0].string))
#define radix_tree_value_type(tree) typeof((tree)->leaves.data[0])

#define radix_tree_create(tree, default_cap)                                                                           \
    {                                                                                                                  \
        (tree)->root.type = RTT_NONE;                                                                                  \
        freelist_init(&(tree)->strings, radix_tree_key_length(tree) > RADIX_TREE_SMALL_STR_SIZE ? default_cap : 1);    \
        freelist_init(&(tree)->branches, (default_cap + 16) / 16);                                                     \
        freelist_init(&(tree)->edges, default_cap);                                                                    \
        freelist_init(&(tree)->leaves, default_cap);                                                                   \
    }

#define radix_tree_destroy(tree)                                                                                       \
    {                                                                                                                  \
        freelist_destroy(&(tree)->strings);                                                                            \
        freelist_destroy(&(tree)->branches);                                                                           \
        freelist_destroy(&(tree)->edges);                                                                              \
        freelist_destroy(&(tree)->leaves);                                                                             \
    }

#define _RT(x, c) ____CONCAT(____CONCAT(____CONCAT(____rt_temp, x), ____), c)

#define ___radix_tree_create_leaf_node(tree, value, c)                                                                 \
    ({                                                                                                                 \
        const __auto_type _RT(tmp, c) = (value);                                                                       \
        const uint32_t idx = freelist_insert(&(tree)->leaves, (_RT(tmp, c)));                                          \
        (struct RadixTreeNodePtr){.type = RTT_LEAF, .idx = idx};                                                       \
    })

#define radix_tree_create_leaf_node(tree, value) ___radix_tree_create_leaf_node(tree, value, __COUNTER__)

#define ___radix_tree_create_branch_node(tree, c)                                                                      \
    ({                                                                                                                 \
        const __auto_type _RT(tmp, c) = ((const struct RadixTreeBranchNode){0});                                       \
        const uint32_t idx = freelist_insert(&(tree)->branches, (_RT(tmp, c)));                                        \
        (struct RadixTreeNodePtr){.type = RTT_BRANCH, .idx = idx};                                                     \
    })

#define radix_tree_create_branch_node(tree) ___radix_tree_create_branch_node(tree, __COUNTER__)

#define ____radix_tree_create_edge_node(tree, key, offset, key_length, next_node, c)                                   \
    ({                                                                                                                 \
        const __auto_type _RT(edge, c) = ((struct RadixTreeEdgeNode){.length = (key_length), .next = (next_node)});    \
        const uint32_t _RT(idx, c) = freelist_insert(&(tree)->edges, (_RT(edge, c)));                                  \
        if (_RT(edge, c).length > RADIX_TREE_SMALL_STR_SIZE) {                                                         \
            struct {                                                                                                   \
                radix_key_t string[radix_tree_key_length(tree)];                                                       \
            } _RT(buf, c);                                                                                             \
            radix_tree_copy_key(_RT(buf, c).string, key, offset, _RT(edge, c).length);                                 \
            (tree)->edges.data[_RT(idx, c)].string_idx =                                                               \
                freelist_insert_unsafe(&(tree)->strings, &_RT(buf, c), radix_tree_key_length(tree));                   \
        } else {                                                                                                       \
            radix_tree_copy_key((tree)->edges.data[_RT(idx, c)].data, key, offset, _RT(edge, c).length);               \
        }                                                                                                              \
        (struct RadixTreeNodePtr){.type = RTT_EDGE, .idx = _RT(idx, c)};                                               \
    })

#define radix_tree_create_edge_node(tree, key, offset, length, next)                                                   \
    ____radix_tree_create_edge_node(tree, key, offset, length, next, __COUNTER__)

#define ____radix_tree_insert(tree, key, value, c)                                                                     \
    {                                                                                                                  \
        struct RadixTreeNodePtr _RT(node, c) = (tree)->root;                                                           \
        struct RadixTreeNodePtr _RT(prev, c) = {.type = RTT_NONE};                                                     \
        radix_key_idx_t _RT(keyidx, c) = 0;                                                                            \
        radix_key_idx_t _RT(previdx, c) = 0;                                                                           \
        if (_RT(node, c).type == RTT_NONE) {                                                                           \
            const struct RadixTreeNodePtr _RT(leaf, c) = radix_tree_create_leaf_node(tree, value);                     \
            (tree)->root = radix_tree_create_edge_node(tree, key, 0, radix_tree_key_length(tree), _RT(leaf, c));       \
            goto _RT(exit, c);                                                                                         \
        }                                                                                                              \
        for (;;) {                                                                                                     \
            switch (_RT(node, c).type) {                                                                               \
            case RTT_EDGE: {                                                                                           \
                struct RadixTreeEdgeNode* _RT(edge, c) = radix_tree_fetch_edge(tree, _RT(node, c));                    \
                const radix_key_t* _RT(edge_key, c) = radix_tree_fetch_edge_str(tree, _RT(edge, c));                   \
                radix_key_t _RT(edge_hash, c) = radix_tree_key_unpack(_RT(edge_key, c), 0);                            \
                radix_key_t _RT(key_hash, c) = radix_tree_key_unpack((key), _RT(keyidx, c)++);                         \
                if (_RT(edge_hash, c) != _RT(key_hash, c)) {                                                           \
                    struct RadixTreeNodePtr _RT(old_node, c) = _RT(edge, c)->next;                                     \
                    struct RadixTreeNodePtr _RT(new_node, c) = radix_tree_create_leaf_node(tree, value);               \
                    if (_RT(edge, c)->length > 1) {                                                                    \
                        _RT(old_node, c) = radix_tree_create_edge_node(                                                \
                            tree, _RT(edge_key, c), 1, _RT(edge, c)->length - 1, _RT(old_node, c)                      \
                        );                                                                                             \
                        radix_tree_refetch_edge(_RT(edge, c), tree, _RT(node, c));                                     \
                        radix_tree_refetch_edge_str(_RT(edge_key, c), tree, _RT(edge, c));                             \
                    }                                                                                                  \
                    if (radix_tree_key_length(tree) - _RT(keyidx, c) > 0) {                                            \
                        _RT(new_node, c) = radix_tree_create_edge_node(                                                \
                            tree, key, _RT(keyidx, c), radix_tree_key_length(tree) - _RT(keyidx, c), _RT(new_node, c)  \
                        );                                                                                             \
                        radix_tree_refetch_edge(_RT(edge, c), tree, _RT(node, c));                                     \
                        radix_tree_refetch_edge_str(_RT(edge_key, c), tree, _RT(edge, c));                             \
                    }                                                                                                  \
                    const struct RadixTreeNodePtr _RT(branch_node, c) = radix_tree_create_branch_node(tree);           \
                    struct RadixTreeBranchNode* _RT(branch, c) = radix_tree_fetch_branch(tree, _RT(branch_node, c));   \
                    _RT(branch, c)->next[_RT(edge_hash, c)] = _RT(old_node, c);                                        \
                    _RT(branch, c)->next[_RT(key_hash, c)] = _RT(new_node, c);                                         \
                    if (_RT(edge, c)->length > RADIX_TREE_SMALL_STR_SIZE) {                                            \
                        uint32_t _RT(old_string_idx, c) = _RT(edge, c)->string_idx;                                    \
                        freelist_remove(&(tree)->strings, _RT(old_string_idx, c));                                     \
                    }                                                                                                  \
                    uint32_t _RT(old_edge_idx, c) = _RT(node, c).idx;                                                  \
                    freelist_remove(&(tree)->edges, _RT(old_edge_idx, c));                                             \
                    switch (_RT(prev, c).type) {                                                                       \
                    case RTT_EDGE: {                                                                                   \
                        radix_tree_fetch_edge(tree, _RT(prev, c))->next = _RT(branch_node, c);                         \
                    } break;                                                                                           \
                    case RTT_BRANCH: {                                                                                 \
                        radix_tree_fetch_branch(tree, _RT(prev, c))->next[_RT(previdx, c)] = _RT(branch_node, c);      \
                    } break;                                                                                           \
                    case RTT_NONE: {                                                                                   \
                        (tree)->root = _RT(branch_node, c);                                                            \
                    } break;                                                                                           \
                    default:                                                                                           \
                        break;                                                                                         \
                    }                                                                                                  \
                    goto _RT(exit, c);                                                                                 \
                }                                                                                                      \
                for (radix_key_idx_t _RT(i, c) = 1; _RT(i, c) < _RT(edge, c)->length; _RT(i, c)++) {                   \
                    _RT(edge_hash, c) = radix_tree_key_unpack(_RT(edge_key, c), _RT(i, c));                            \
                    _RT(key_hash, c) = radix_tree_key_unpack(key, _RT(keyidx, c));                                     \
                    if (_RT(edge_hash, c) == _RT(key_hash, c)) {                                                       \
                        _RT(keyidx, c)++;                                                                              \
                        continue;                                                                                      \
                    }                                                                                                  \
                    struct RadixTreeNodePtr _RT(old_node, c) = _RT(edge, c)->next;                                     \
                    struct RadixTreeNodePtr _RT(new_node, c) = radix_tree_create_leaf_node(tree, value);               \
                    if (_RT(edge, c)->length - _RT(i, c) > 1) {                                                        \
                        _RT(old_node, c) = radix_tree_create_edge_node(                                                \
                            tree, _RT(edge_key, c), _RT(i, c) + 1, _RT(edge, c)->length - _RT(i, c) - 1,               \
                            _RT(old_node, c)                                                                           \
                        );                                                                                             \
                        radix_tree_refetch_edge(_RT(edge, c), tree, _RT(node, c));                                     \
                        radix_tree_refetch_edge_str(_RT(edge_key, c), tree, _RT(edge, c));                             \
                    }                                                                                                  \
                    if (radix_tree_key_length(tree) - _RT(keyidx, c) > 1) {                                            \
                        _RT(new_node, c) = radix_tree_create_edge_node(                                                \
                            tree, key, _RT(keyidx, c) + 1, radix_tree_key_length(tree) - _RT(keyidx, c) - 1,           \
                            _RT(new_node, c)                                                                           \
                        );                                                                                             \
                        radix_tree_refetch_edge(_RT(edge, c), tree, _RT(node, c));                                     \
                        radix_tree_refetch_edge_str(_RT(edge_key, c), tree, _RT(edge, c));                             \
                    }                                                                                                  \
                    if (_RT(edge, c)->length > RADIX_TREE_SMALL_STR_SIZE && _RT(i, c) <= RADIX_TREE_SMALL_STR_SIZE) {  \
                        const uint32_t _RT(old_str_idx, c) = _RT(edge, c)->string_idx;                                 \
                        radix_tree_copy_key(_RT(edge, c)->data, _RT(edge_key, c), 0, _RT(i, c));                       \
                        freelist_remove(&(tree)->strings, _RT(old_str_idx, c));                                        \
                    }                                                                                                  \
                    _RT(edge, c)->next = radix_tree_create_branch_node(tree);                                          \
                    _RT(edge, c)->length = _RT(i, c);                                                                  \
                    struct RadixTreeBranchNode* _RT(branch, c) = radix_tree_fetch_branch(tree, _RT(edge, c)->next);    \
                    _RT(branch, c)->next[_RT(edge_hash, c)] = _RT(old_node, c);                                        \
                    _RT(branch, c)->next[_RT(key_hash, c)] = _RT(new_node, c);                                         \
                    goto _RT(exit, c);                                                                                 \
                }                                                                                                      \
                _RT(prev, c) = _RT(node, c);                                                                           \
                _RT(node, c) = _RT(edge, c)->next;                                                                     \
            } break;                                                                                                   \
            case RTT_BRANCH: {                                                                                         \
                struct RadixTreeBranchNode* _RT(branch, c) = radix_tree_fetch_branch(tree, _RT(node, c));              \
                const radix_key_t _RT(key_branch, c) = radix_tree_key_unpack(key, _RT(keyidx, c)++);                   \
                const struct RadixTreeNodePtr _RT(next, c) = _RT(branch, c)->next[_RT(key_branch, c)];                 \
                if (_RT(next, c).type == RTT_NONE) {                                                                   \
                    struct RadixTreeNodePtr _RT(new_node, c) = radix_tree_create_leaf_node(tree, value);               \
                    if (radix_tree_key_length(tree) - _RT(keyidx, c) > 0) {                                            \
                        _RT(new_node, c) = radix_tree_create_edge_node(                                                \
                            tree, key, _RT(keyidx, c), radix_tree_key_length(tree) - _RT(keyidx, c), _RT(new_node, c)  \
                        );                                                                                             \
                    }                                                                                                  \
                    _RT(branch, c)->next[_RT(key_branch, c)] = _RT(new_node, c);                                       \
                    goto _RT(exit, c);                                                                                 \
                }                                                                                                      \
                _RT(prev, c) = _RT(node, c);                                                                           \
                _RT(previdx, c) = _RT(key_branch, c);                                                                  \
                _RT(node, c) = _RT(next, c);                                                                           \
            } break;                                                                                                   \
            default: {                                                                                                 \
                goto _RT(exit, c);                                                                                     \
            } break;                                                                                                   \
            }                                                                                                          \
        }                                                                                                              \
        _RT(exit, c) :                                                                                                 \
    }

#define radix_tree_insert(tree, key, value) ____radix_tree_insert(tree, key, value, __COUNTER__)

#define ____radix_tree_get(tree, key, c)                                                                               \
    ({                                                                                                                 \
        radix_tree_value_type(tree) _RT(ret, c);                                                                       \
        memset(&_RT(ret, c), 0, sizeof(_RT(ret, c)));                                                                  \
        struct RadixTreeNodePtr _RT(node, c) = (tree)->root;                                                           \
        radix_key_idx_t _RT(keyidx, c) = 0;                                                                            \
        for (;;) {                                                                                                     \
            switch (_RT(node, c).type) {                                                                               \
            case RTT_EDGE: {                                                                                           \
                const struct RadixTreeEdgeNode* _RT(edge, c) = radix_tree_fetch_edge(tree, _RT(node, c));              \
                const radix_key_t* _RT(edge_key, c) = radix_tree_fetch_edge_str(tree, _RT(edge, c));                   \
                for (radix_key_idx_t _RT(i, c) = 0; _RT(i, c) < _RT(edge, c)->length; _RT(i, c)++) {                   \
                    if (radix_tree_key_unpack(_RT(edge_key, c), _RT(i, c)) !=                                          \
                        radix_tree_key_unpack(key, _RT(keyidx, c)++)) {                                                \
                        goto _RT(end, c);                                                                              \
                    }                                                                                                  \
                }                                                                                                      \
                _RT(node, c) = _RT(edge, c)->next;                                                                     \
            } break;                                                                                                   \
            case RTT_BRANCH: {                                                                                         \
                const struct RadixTreeBranchNode* _RT(branch, c) = radix_tree_fetch_branch(tree, _RT(node, c));        \
                const radix_key_t _RT(key_branch, c) = radix_tree_key_unpack(key, _RT(keyidx, c)++);                   \
                _RT(node, c) = _RT(branch, c)->next[_RT(key_branch, c)];                                               \
            } break;                                                                                                   \
            case RTT_LEAF: {                                                                                           \
                _RT(ret, c) = *radix_tree_fetch_leaf(tree, _RT(node, c));                                              \
                goto _RT(end, c);                                                                                      \
            } break;                                                                                                   \
            default: {                                                                                                 \
                goto _RT(end, c);                                                                                      \
            } break;                                                                                                   \
            }                                                                                                          \
        }                                                                                                              \
        _RT(end, c) : _RT(ret, c);                                                                                     \
    })

#define radix_tree_get(tree, key) ____radix_tree_get(tree, key, __COUNTER__)

static void radix_tree_debug_node(
    void* tree, const struct RadixTreeNodePtr node, FILE* stream, const uint32_t indent, const uint32_t key_length,
    const uint32_t value_length
) {
    const RadixTree(void*, 0)* radix_tree = tree;

    switch (node.type) {
    case RTT_BRANCH: {
        const struct RadixTreeBranchNode* branch = radix_tree_fetch_branch(radix_tree, node);
        for (uint32_t i = 0; i < RADIX_TREE_KEY_BRANCHES; i++) {
            if (branch->next[i].type != RTT_NONE) {
                fprintf(stream, "%*s[%x]\n", indent + 2, "", i);
                radix_tree_debug_node(tree, branch->next[i], stream, indent + 2, key_length, value_length);
            }
        }
    } break;
    case RTT_EDGE: {
        const struct RadixTreeEdgeNode* edge = radix_tree_fetch_edge(radix_tree, node);
        fprintf(stream, "%*sEdge\n", indent, "");
        fprintf(stream, "%*sLength: %d\n", indent + 2, "", edge->length);
        fprintf(stream, "%*sData: ", indent + 2, "");
        const radix_key_t* edge_str = radix_tree_fetch_edge_str(radix_tree, edge);
        for (uint32_t i = 0; i < edge->length; i++) {
            fprintf(stream, "%01x", radix_tree_key_unpack(edge_str, i));
        }
        fprintf(stream, "\n");
        if (edge->next.type == RTT_NONE) {
            return;
        }
        radix_tree_debug_node(tree, edge->next, stream, indent + 2, key_length, value_length);
    } break;
    case RTT_LEAF: {
        const void* leaf = radix_tree_fetch_leaf(radix_tree, node);
        fprintf(stream, "%*sLeaf\n", indent, "");
        fprintf(stream, "%*sValue: %p\n", indent + 2, "", leaf);
    } break;
    case RTT_NONE: {
        fprintf(stream, "%*sNone\n", indent, "");
    } break;
    }
}

#define radix_tree_debug(tree, stream)                                                                                 \
    radix_tree_debug_node(                                                                                             \
        (void*)tree, (tree)->root, stream, 0, radix_tree_key_length(tree), sizeof(radix_tree_value_type(tree))         \
    )