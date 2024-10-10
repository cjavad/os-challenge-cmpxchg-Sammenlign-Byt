//
// Created by javad on 10-10-24.
//

#include "cache.h"

#include <assert.h>
#include <string.h>

Cache* cache_create(const uint32_t default_cap) {
    Cache* cache = calloc(1, sizeof(Cache));

    cache->lock = (pthread_mutex_t)PTHREAD_MUTEX_INITIALIZER;

    freelist_init(&cache->strings, default_cap);
    freelist_init(&cache->branches, default_cap);
    freelist_init(&cache->edges, default_cap);
    freelist_init(&cache->leaves, default_cap);

    cache->root = (TreeNodePointer){.type = TT_NONE, .idx = 0};

    return cache;
}

void cache_destroy(Cache* cache) {
    freelist_destroy(&cache->strings);
    freelist_destroy(&cache->branches);
    freelist_destroy(&cache->edges);
    freelist_destroy(&cache->leaves);
    free(cache);
}

uint8_t* cache_get_edge_str(const Cache* cache, struct TreeNodeEdge* edge) {
    if (edge->length > 4) {
        return cache->strings.data[edge->str_idx].str;
    }

    return edge->data;
}

uint64_t cache_get(Cache* cache, HashDigest key) {
    pthread_mutex_lock(&cache->lock);

    TreeNodePointer* node = &cache->root;
    uint8_t key_idx = 0;

    if (node->type == TT_NONE) {
        goto notfound;
    }

    for (;;) {
        switch (node->type) {
        case TT_EDGE: {
            struct TreeNodeEdge* edge_node = &cache->edges.data[node->idx];
            const uint8_t* edge_key = cache_get_edge_str(cache, edge_node);

            for (uint8_t i = 0; i < edge_node->length; i++) {
                if (edge_key[i] != cache_key_hash(key, key_idx++)) {
                    goto notfound;
                }
            }

            *node = edge_node->next;
        }
        break;
        case TT_BRANCH: {
            const struct TreeNodeBranch* branch_node = &cache->branches.data[node->idx];
            const uint8_t hash = cache_key_hash(key, key_idx++);
            const TreeNodePointer next = branch_node->next[hash];

            if (next.type == 0) {
                goto notfound;
            }

            *node = next;
        }
        break;
        case TT_LEAF: {
            const struct TreeNodeLeaf* leaf = &cache->leaves.data[node->idx];
            pthread_mutex_unlock(&cache->lock);
            return leaf->value;
        }
        break;
        default: {
            goto notfound;
        }
        }
    }

notfound:
    pthread_mutex_unlock(&cache->lock);
    return 0;
}

TreeNodePointer cache_create_leaf_node(Cache* cache, const uint64_t value) {
    const struct TreeNodeLeaf leaf = {.value = value};
    const uint32_t idx = freelist_add(&cache->leaves, leaf);
    return (TreeNodePointer){.type = TT_LEAF, .idx = idx};
}

TreeNodePointer cache_create_edge_node(Cache* cache, const uint8_t* key, const uint32_t offset, const uint32_t end,
                                       const TreeNodePointer next, bool packed) {
    const struct TreeNodeEdge edge = {.length = end - offset, .next = next};

    const uint32_t idx = freelist_add(&cache->edges, edge);

    uint8_t* buffer;

    if (edge.length > 4) {
        const uint32_t str_idx = freelist_add(&cache->strings, (union TreeNodeKeyEntry){0});
        cache->edges.data[idx].str_idx = str_idx;
        buffer = cache->strings.data[str_idx].str;
    } else {
        buffer = cache->edges.data[idx].data;
    }

    if (packed) {
        memcpy(buffer, key + offset, edge.length);
    } else {
        // Unpack and write into the data
        for (uint8_t i = 0; i < edge.length; i++) {
            buffer[i] = cache_key_hash(key, offset + i);
        }
    }

    return (TreeNodePointer){.type = TT_EDGE, .idx = idx};
}

TreeNodePointer cache_create_branch_node(Cache* cache) {
    const struct TreeNodeBranch branch = {0};
    const uint32_t idx = freelist_add(&cache->branches, branch);
    return (TreeNodePointer){.type = TT_BRANCH, .idx = idx};
}


void cache_insert(Cache* cache, HashDigest key, const uint64_t value) {
    pthread_mutex_lock(&cache->lock);

    // Current node
    TreeNodePointer* node = &cache->root;
    // Index into key (0-CACHE_KEY_LENGTH)
    uint8_t key_idx = 0;

    // If the root node is empty insert a new edge with
    // the entire key and a leaf node with the value.
    if (node->type == TT_NONE) {
        const TreeNodePointer leaf = cache_create_leaf_node(cache, value);
        cache->root = cache_create_edge_node(cache, key, 0, CACHE_KEY_LENGTH, leaf, false);
        goto exit;
    }

    for (;;) {
        switch (node->type) {
        case TT_EDGE: {
            struct TreeNodeEdge* edge_node = &cache->edges.data[node->idx];
            const uint8_t* edge_key = cache_get_edge_str(cache, edge_node);

            // First character does not match insert branch
            if (edge_key[0] != cache_key_hash(key, key_idx)) {

                goto exit;
            }

            // Skip next index as we already know it matches
            key_idx++;

            for (uint8_t i = 1; i < edge_node->length; i++) {
                const uint8_t old_hash = edge_key[i];
                const uint8_t new_hash = cache_key_hash(key, key_idx);

                // Skip over matching indices as long as possible
                // But don't consume the first one that doesn't match
                if (old_hash == new_hash) {
                    key_idx++;
                    continue;
                }

                // Found a mismatch, insert a branch.
                TreeNodePointer old_node = edge_node->next;
                TreeNodePointer new_node = cache_create_leaf_node(cache, value);

                // Create new edge IF there are characters left in the edge
                // otherwise directly insert the leaf.
                if (edge_node->length - i - 1 > 0) {
                    old_node = cache_create_edge_node(cache, edge_key, i + 1, edge_node->length - i - 1, old_node,
                                                      true);
                }

                // Also only create a edge if we have more characters left in the key
                if (key_idx + 1 < CACHE_KEY_LENGTH) {
                    new_node = cache_create_edge_node(cache, key, key_idx + 1, CACHE_KEY_LENGTH - key_idx - 1,
                                                      new_node, false);
                }

                const TreeNodePointer branch = cache_create_branch_node(cache);
                struct TreeNodeBranch* branch_node = &cache->branches.data[branch.idx];
                branch_node->next[old_hash] = old_node;
                branch_node->next[new_hash] = new_node;

                edge_node->length = i;
                edge_node->next = branch;

                goto exit;
            }

            *node = edge_node->next;
        }
        break;
        case TT_BRANCH: {
            struct TreeNodeBranch* branch_node = &cache->branches.data[node->idx];
            const uint8_t hash = cache_key_hash(key, key_idx++);
            const TreeNodePointer next = branch_node->next[hash];

            // Just insert rest as edge + leaf into the branch
            if (next.type == 0) {
                const TreeNodePointer leaf = cache_create_leaf_node(cache, value);
                const TreeNodePointer edge = cache_create_edge_node(cache, key, key_idx, CACHE_KEY_LENGTH - key_idx,
                                                                    leaf, false);
                branch_node->next[hash] = edge;
                goto exit;
            }

            *node = next;
        }
        break;

        default:
            goto exit;
        }
    }

exit:
    pthread_mutex_unlock(&cache->lock);
}