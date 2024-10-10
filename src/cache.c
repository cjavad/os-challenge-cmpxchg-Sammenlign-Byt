//
// Created by javad on 10-10-24.
//

#include "cache.h"

#include <assert.h>
#include <string.h>

Cache* cache_create() {
    Cache* cache = calloc(1, sizeof(Cache));

    cache->lock = (pthread_mutex_t)PTHREAD_MUTEX_INITIALIZER;
    vec_init(&cache->nodes, 1024);

    const TreeNode root = {.type = TT_NONE};
    vec_push(&cache->nodes, root);

    return cache;
}

void cache_destroy(Cache* cache) {
    vec_destroy(&cache->nodes);
    free(cache);
}

uint64_t cache_get(Cache* cache, HashDigest key) {
    pthread_mutex_lock(&cache->lock);

    const TreeNode* node = &vec_get(&cache->nodes, 0);
    uint32_t key_idx = 0;

    if (node->type == TT_NONE) {
        goto notfound;
    }

    while (true) {
        switch (node->type) {
        case TT_BRANCH: {
            const uint32_t hex_char = cache_key_hash_idx(key, key_idx++);
            const uint32_t next = node->branch.next[hex_char];

            if (next == 0) {
                goto notfound;
            }

            node = &vec_get(&cache->nodes, next);
        } break;
        case TT_EDGE: {
            assert(node->edge.length <= CACHE_KEY_LENGTH - key_idx);

            for (uint64_t i = 0; i < node->edge.length; i++) {
                const uint32_t hex_char = cache_key_hash_idx(key, key_idx++);

                if (node->edge.data[i] != hex_char) {
                    goto notfound;
                }
            }

            const uint32_t next = node->edge.next;

            assert(next != 0);

            node = &vec_get(&cache->nodes, next);
        } break;
        case TT_LEAF: {
            pthread_mutex_unlock(&cache->lock);
            return node->leaf.value;
        } break;
        case TT_NONE: {
            // Unreachable.
            assert(false);
        }
        }
    }

notfound:
    pthread_mutex_unlock(&cache->lock);
    return 0;
}

uint32_t cache_create_leaf(Cache* cache, uint64_t value) {
    const uint32_t idx = cache->nodes.len;
    const TreeNode n = {.type = TT_LEAF, .leaf.value = value};
    vec_push(&cache->nodes, n);
    return idx;
}

uint32_t cache_create_edge(Cache* cache, const uint8_t* hex_key, const uint32_t offset, const uint32_t next_node) {
    TreeNode edge = {.type = TT_EDGE};
    edge.edge.length = CACHE_KEY_LENGTH - offset;
    memcpy(&edge.edge.data, hex_key + offset, edge.edge.length);

    const uint32_t edge_idx = cache->nodes.len;
    vec_push(&cache->nodes, edge);

    cache->nodes.data[edge_idx].edge.next = next_node;

    return edge_idx;
}

uint32_t cache_create_edge_with_leaf(Cache* cache, uint8_t* hex_key, uint32_t offset, uint64_t value) {
    const uint32_t leaf_idx = cache_create_leaf(cache, value);
    return cache_create_edge(cache, hex_key, offset, leaf_idx);
}

#define REFETCH_NODE(cache, node, idx) node = &vec_get(&cache->nodes, idx);

void cache_insert(Cache* cache, HashDigest key, const uint64_t value) {
    pthread_mutex_lock(&cache->lock);

    uint32_t node_idx = 0;
    TreeNode* node;

    REFETCH_NODE(cache, node, node_idx);

    uint32_t key_idx = 0;
    uint8_t hex_key[CACHE_KEY_LENGTH];

    for (uint64_t i = 0; i < CACHE_KEY_LENGTH; i++) {
        hex_key[i] = cache_key_hash_idx(key, i);
    }

    // Base case, no root node yet
    // Just add single entry as edge with entire key.
    if (node->type == TT_NONE) {
        bzero(node, sizeof(TreeNode));
        node->type = TT_EDGE;
        node->edge.length = (CACHE_KEY_LENGTH - key_idx);
        memcpy(node->edge.data, &hex_key, node->edge.length);
        const uint32_t leaf_idx = cache_create_leaf(cache, value);
        REFETCH_NODE(cache, node, node_idx);
        node->edge.next = leaf_idx;
        goto exit;
    }

    while (true) {
        switch (node->type) {
        case TT_BRANCH: {
            const uint32_t hex = hex_key[key_idx++];
            const uint32_t next = node->branch.next[hex];

            // Insert edge and leaf.
            if (next == 0) {
                // Creates edge with leaf node
                // add to next map on current branch node.
                const uint32_t edge_idx = cache_create_edge_with_leaf(cache, hex_key, key_idx, value);
                REFETCH_NODE(cache, node, node_idx);
                node->branch.next[hex] = edge_idx;
                goto exit;
            }

            node_idx = next;
            REFETCH_NODE(cache, node, node_idx);
        } break;
        case TT_EDGE: {
            // First hex char does NOT match
            if (node->edge.data[0] != hex_key[key_idx]) {
                uint32_t old_node = node->edge.next;
                uint32_t new_node = cache_create_leaf(cache, value);
                REFETCH_NODE(cache, node, node_idx);

                const uint8_t current_hex = hex_key[key_idx++];
                const uint8_t edge_current_hex = node->edge.data[0];

                // If current edge has remaining bytes create new edge with remainder
                // other just add the leaf in this branch node.
                if (node->edge.length != 1) {
                    const uint32_t old_node_new_edge_idx = cache->nodes.len;

                    TreeNode edge = {.type = TT_EDGE, .edge = {.next = old_node, .length = node->edge.length - 1}};

                    memcpy(edge.edge.data, node->edge.data + 1, edge.edge.length);

                    vec_push(&cache->nodes, edge);
                    REFETCH_NODE(cache, node, node_idx);

                    old_node = old_node_new_edge_idx;
                }

                // Do the same for the new key we are inserting not creating empty edges.
                if (key_idx < CACHE_KEY_LENGTH) {
                    new_node = cache_create_edge(cache, hex_key, key_idx, new_node);
                    REFETCH_NODE(cache, node, node_idx);
                }

                bzero(node, sizeof(TreeNode));
                node->type = TT_BRANCH;
                node->branch.next[edge_current_hex] = old_node;
                node->branch.next[current_hex] = new_node;

                goto exit;
            }

            // Keep edge.data until no longer matches.
            for (uint32_t i = 1; i < node->edge.length; i++) {

                // From this point on the keys differ
                // Create two separate edges and truncate current
                // until this point.
                if (node->edge.data[i] != hex_key[key_idx]) {
                    uint32_t old_node = node->edge.next;
                    uint32_t new_node = cache_create_leaf(cache, value);
                    REFETCH_NODE(cache, node, node_idx);

                    // Create new edge if there are more key bytes in old node
                    // otherwise we just add the leaf directly.
                    if (node->edge.length - i - 1 > 0) {
                        const uint32_t old_node_new_edge_idx = cache->nodes.len;

                        TreeNode edge = {
                            .type = TT_EDGE, .edge = {.next = old_node, .length = node->edge.length - i - 1}
                        };

                        memcpy(edge.edge.data, node->edge.data + i + 1, edge.edge.length);

                        vec_push(&cache->nodes, edge);
                        REFETCH_NODE(cache, node, node_idx);

                        old_node = old_node_new_edge_idx;
                    }

                    // If we have remaining key data from our new inserted key
                    // create an edge to hold it.
                    if (key_idx + 1 < CACHE_KEY_LENGTH) {
                        new_node = cache_create_edge(cache, hex_key, key_idx + 1, new_node);
                        REFETCH_NODE(cache, node, node_idx);
                    }

                    // New branch to set as new node for current edge.
                    TreeNode branch = {.type = TT_BRANCH};
                    branch.branch.next[node->edge.data[i]] = old_node;
                    branch.branch.next[hex_key[key_idx]] = new_node;

                    const uint32_t branch_idx = cache->nodes.len;
                    vec_push(&cache->nodes, branch);
                    REFETCH_NODE(cache, node, node_idx);

                    node->edge.length = i;
                    node->edge.next = branch_idx;

                    goto exit;
                }

                // Only consume matching keys.
                key_idx++;
            }

            node_idx = node->edge.next;
            REFETCH_NODE(cache, node, node_idx);
        } break;
        case TT_LEAF: {
            assert(false);
        } break;
        case TT_NONE: {
            // Unreachable.
            assert(false);
        }
        }
    }

exit:
    pthread_mutex_unlock(&cache->lock);
}