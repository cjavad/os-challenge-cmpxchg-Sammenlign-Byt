#include "radix_tree.h"

inline struct RadixTreeNodePtr radix_tree_create_leaf_node(
    _RadixTreeBase* tree, const void* value_ptr, const uint32_t value_size
) {
    const uint32_t idx = freelist_insert_unsafe(&(tree)->leaves, value_ptr, value_size);
    return (struct RadixTreeNodePtr){.type = RTT_LEAF, .idx = idx};
}

inline struct RadixTreeNodePtr radix_tree_create_branch_node(
    _RadixTreeBase* tree, const enum RadixTreeNodeType branch_type
) {
    switch (branch_type) {

    case RTT_BRANCH_4:
        return (struct RadixTreeNodePtr
        ){.type = branch_type, .idx = freelist_insert(&tree->branches4, (const struct RadixTreeBranch4Node){0})};
    case RTT_BRANCH_8:
        return (struct RadixTreeNodePtr
        ){.type = branch_type, .idx = freelist_insert(&tree->branches8, (const struct RadixTreeBranch8Node){0})};
    case RTT_BRANCH_16:
        return (struct RadixTreeNodePtr
        ){.type = branch_type, .idx = freelist_insert(&tree->branches16, (const struct RadixTreeBranch16Node){0})};
    case RTT_BRANCH_FULL:
        return (struct RadixTreeNodePtr
        ){.type = branch_type, .idx = freelist_insert(&tree->branches_full, (const struct RadixTreeBranchFullNode){0})};
    default:
        __builtin_unreachable();
    }
}

inline struct RadixTreeNodePtr radix_tree_create_edge_node(
    _RadixTreeBase* tree, const radix_key_t* key, const radix_key_idx_t key_size, const radix_key_idx_t offset,
    const radix_key_idx_t key_length, const struct RadixTreeNodePtr next
) {
    const struct RadixTreeEdgeNode edge = {.length = key_length, .next = next};

    // Always copy out the key from the buffer before inserting a new edge
    // that potentially invalidates the buffer.
    radix_key_t buf[key_length];
    radix_tree_copy_key(buf, key, offset, edge.length);
    const uint32_t idx = freelist_insert(&tree->edges, edge);

    if (edge.length > RADIX_TREE_SMALL_STR_SIZE) {
#if RADIX_TREE_MALLOC_STRS
        tree->edges.data[idx].string_ptr = malloc((edge.length + 1) / RADIX_TREE_KEY_RATIO);
        memcpy(tree->edges.data[idx].string_ptr, buf, (edge.length + 1) / RADIX_TREE_KEY_RATIO);
#else
        tree->edges.data[idx].string_idx = freelist_insert_unsafe(&tree->strings, buf, key_size);
#endif
    } else {
        memcpy(tree->edges.data[idx].data, buf, (edge.length + 1) / RADIX_TREE_KEY_RATIO);
    }
    return (struct RadixTreeNodePtr){.type = RTT_EDGE, .idx = idx};
}

inline void radix_tree_get_branch(
    const _RadixTreeBase* tree, const struct RadixTreeNodePtr* node, const radix_key_t key, struct RadixTreeNodePtr* out
) {
    switch (node->type) {
    case RTT_BRANCH_4: {
        radix_tree_get_branch4(radix_tree_fetch_branch4(tree, *node), key, out);
    } break;
    case RTT_BRANCH_8: {
        radix_tree_get_branch8(radix_tree_fetch_branch8(tree, *node), key, out);
    } break;
    case RTT_BRANCH_16: {
        radix_tree_get_branch_16(radix_tree_fetch_branch16(tree, *node), key, out);
    } break;
    case RTT_BRANCH_FULL: {
        radix_tree_get_branch_full(radix_tree_fetch_branch_full(tree, *node), key, out);
    } break;
    default:
        __builtin_unreachable();
    }
}

inline void radix_tree_get_branch4(
    const struct RadixTreeBranch4Node* branch, const radix_key_t key, struct RadixTreeNodePtr* node
) {
    for (radix_key_idx_t i = 0; i < 4; i++) {
        if (branch->values[i] == key) {
            *node = branch->entries[i];
            return;
        }
    }

    *node = (struct RadixTreeNodePtr){.type = RTT_NONE};
}

inline void radix_tree_get_branch8(
    const struct RadixTreeBranch8Node* branch, const radix_key_t key, struct RadixTreeNodePtr* node
) {
    for (radix_key_idx_t i = 0; i < 8; i++) {
        if (branch->values[i] == key) {
            *node = branch->entries[i];
            return;
        }
    }

    *node = (struct RadixTreeNodePtr){.type = RTT_NONE};
}

inline void radix_tree_get_branch_16(
    const struct RadixTreeBranch16Node* branch, const radix_key_t key, struct RadixTreeNodePtr* node
) {
    if (key == RADIX_BRANCH_IMMEDIATE) {
        *node = (struct RadixTreeNodePtr){.type = RTT_NONE};
        return;
    }

    *node = branch->entries[key];
}

inline void radix_tree_get_branch_full(
    const struct RadixTreeBranchFullNode* branch, const radix_key_t key, struct RadixTreeNodePtr* node
) {
    *node = branch->entries[key];
}

inline void radix_tree_set_prev(_RadixTreeBase* tree, const struct RadixTreeNodePtr new_node) {
    struct RadixTreeNodePtr node = new_node;
    uint32_t idx = tree->prev.len - 1;

    for (;;) {
        const struct RadixTreePrevPtr* prev = &tree->prev.data[idx];

        switch (prev->node.type) {
        case RTT_NONE: {
            tree->root = node;
            return;
        } break;
        case RTT_EDGE: {
            struct RadixTreeEdgeNode* edge = radix_tree_fetch_edge(tree, prev->node);
            edge->next = node;
            return;
        } break;
        case RTT_BRANCH_4:
        case RTT_BRANCH_8:
        case RTT_BRANCH_16:
        case RTT_BRANCH_FULL: {
            node = radix_tree_insert_into_branch_or_grow(tree, prev->node, prev->branch_val, node);

            if (node.type == RTT_NONE) {
                return;
            }

            idx--;
        } break;
        default:
            __builtin_unreachable();
        }
    }
}

inline struct RadixTreeNodePtr radix_tree_insert_into_branch_or_grow(
    _RadixTreeBase* tree, const struct RadixTreeNodePtr branch_node, const radix_key_t key,
    const struct RadixTreeNodePtr node
) {
    switch (branch_node.type) {
    case RTT_BRANCH_4: {
        struct RadixTreeBranch4Node* branch = radix_tree_fetch_branch4(tree, branch_node);
        const struct RadixTreeNodePtr new_node = radix_tree_insert_into_branch4_or_grow(tree, branch, key, node);

        if (new_node.type != RTT_NONE) {
            freelist_remove(&tree->branches4, branch_node.idx);
            return new_node;
        }

    } break;
    case RTT_BRANCH_8: {
        struct RadixTreeBranch8Node* branch = radix_tree_fetch_branch8(tree, branch_node);
        const struct RadixTreeNodePtr new_node = radix_tree_insert_into_branch8_or_grow(tree, branch, key, node);

        if (new_node.type != RTT_NONE) {
            freelist_remove(&tree->branches8, branch_node.idx);
            return new_node;
        }

    } break;
    case RTT_BRANCH_16: {
        struct RadixTreeBranch16Node* branch = radix_tree_fetch_branch16(tree, branch_node);
        const struct RadixTreeNodePtr new_node = radix_tree_insert_into_branch16_or_grow(tree, branch, key, node);

        if (new_node.type != RTT_NONE) {
            freelist_remove(&tree->branches16, branch_node.idx);
            return new_node;
        }

    } break;
    case RTT_BRANCH_FULL: {
        struct RadixTreeBranchFullNode* branch = radix_tree_fetch_branch_full(tree, branch_node);
        radix_tree_insert_into_branch_full(tree, branch, key, node);
    } break;
    default: {
        __builtin_unreachable();
    }
    }

    return (struct RadixTreeNodePtr){0};
}

inline struct RadixTreeNodePtr radix_tree_insert_into_branch4_or_grow(
    _RadixTreeBase* tree, struct RadixTreeBranch4Node* branch, const radix_key_t key, const struct RadixTreeNodePtr node
) {
    for (radix_key_idx_t i = 0; i < 4; i++) {
        if (branch->entries[i].type != RTT_NONE && branch->values[i] != key) {
            continue;
        }

        branch->values[i] = key;
        branch->entries[i] = node;

        return (struct RadixTreeNodePtr){0};
    }

    struct RadixTreeBranch8Node new_branch = {};

    for (radix_key_idx_t i = 0; i < 4; i++) {
        new_branch.values[i] = branch->values[i];
        new_branch.entries[i] = branch->entries[i];
    }

    new_branch.values[4] = key;
    new_branch.entries[4] = node;

    const uint32_t idx = freelist_insert(&tree->branches8, new_branch);

    return (struct RadixTreeNodePtr){.type = RTT_BRANCH_8, .idx = idx};
}

inline struct RadixTreeNodePtr radix_tree_insert_into_branch8_or_grow(
    _RadixTreeBase* tree, struct RadixTreeBranch8Node* branch, const radix_key_t key, const struct RadixTreeNodePtr node
) {
    // We need to keep track of if we've seen an immediate value
    bool seen_immediate = key == RADIX_BRANCH_IMMEDIATE;

    for (radix_key_idx_t i = 0; i < 8; i++) {
        if (branch->entries[i].type != RTT_NONE && branch->values[i] != key) {
            seen_immediate |= branch->values[i] == RADIX_BRANCH_IMMEDIATE;
            continue;
        }

        branch->values[i] = key;
        branch->entries[i] = node;

        return (struct RadixTreeNodePtr){0};
    }

    // Grow and add.
    if (seen_immediate) {
        struct RadixTreeBranchFullNode new_branch = {};

        for (radix_key_idx_t i = 0; i < 8; i++) {
            new_branch.entries[branch->values[i]] = branch->entries[i];
        }

        new_branch.entries[key] = node;

        const uint32_t idx = freelist_insert(&tree->branches_full, new_branch);

        return (struct RadixTreeNodePtr){.type = RTT_BRANCH_FULL, .idx = idx};
    }

    struct RadixTreeBranch16Node new_branch = {};

    for (radix_key_idx_t i = 0; i < 8; i++) {
        new_branch.entries[branch->values[i]] = branch->entries[i];
    }

    new_branch.entries[key] = node;

    const uint32_t idx = freelist_insert(&tree->branches16, new_branch);

    return (struct RadixTreeNodePtr){.type = RTT_BRANCH_16, .idx = idx};
}

inline struct RadixTreeNodePtr radix_tree_insert_into_branch16_or_grow(
    _RadixTreeBase* tree, struct RadixTreeBranch16Node* branch, const radix_key_t key,
    const struct RadixTreeNodePtr node
) {
    // If we insert an immediate now we have to become a full branch.
    if (key == RADIX_BRANCH_IMMEDIATE) {
        struct RadixTreeBranchFullNode new_branch = {};
        memcpy(&new_branch.entries, branch->entries, sizeof(struct RadixTreeBranch16Node));
        new_branch.entries[RADIX_BRANCH_IMMEDIATE] = node;

        const uint32_t idx = freelist_insert(&tree->branches_full, new_branch);

        return (struct RadixTreeNodePtr){.type = RTT_BRANCH_FULL, .idx = idx};
    }

    branch->entries[key] = node;

    return (struct RadixTreeNodePtr){0};
}

inline void radix_tree_insert_into_branch_full(
    _RadixTreeBase* tree, struct RadixTreeBranchFullNode* branch, const radix_key_t key,
    const struct RadixTreeNodePtr node
) {
    branch->entries[key] = node;
}

inline void radix_tree_push_prev(
    _RadixTreeBase* tree, const struct RadixTreeNodePtr node, const radix_key_idx_t branch_val
) {
    if (node.type == RTT_EDGE) {
        tree->prev.len = 0;
    }

    const struct RadixTreePrevPtr new_prev = {.node = node, .branch_val = branch_val};
    vec_push(&tree->prev, new_prev);
}

void _radix_tree_insert(
    _RadixTreeBase* tree, const radix_key_t* new_key, const radix_key_idx_t key_len, const radix_key_idx_t key_size,
    const void* value, const uint32_t value_size
) {
    struct RadixTreeNodePtr node = tree->root;

    tree->prev.len = 0;
    radix_tree_push_prev(tree, (struct RadixTreeNodePtr){.type = RTT_NONE}, RADIX_BRANCH_IMMEDIATE);

    radix_key_idx_t key_idx = 0;

    for (;;) {
        switch (node.type) {
        case RTT_EDGE: {
            struct RadixTreeEdgeNode* edge = radix_tree_fetch_edge(tree, node);
            const radix_key_t* old_key = radix_tree_fetch_edge_str_unsafe(tree, edge, key_size);
            radix_key_idx_t old_idx = 0;

            bool old_has_remainder = true;
            bool new_has_remainder = true;

            radix_key_t old_val = radix_tree_key_unpack(old_key, old_idx++);
            radix_key_t new_val = radix_tree_key_unpack(new_key, key_idx++);

            // First key idx does not match so we branch on them.
            if (old_val != new_val) {
                struct RadixTreeNodePtr old_node = edge->next;
                struct RadixTreeNodePtr new_node = radix_tree_create_leaf_node(tree, value, value_size);

                if (edge->length > 1) {
                    old_node = radix_tree_create_edge_node(tree, old_key, key_size, 1, edge->length - 1, old_node);
                }

                if (key_len - key_idx > 0) {
                    new_node =
                        radix_tree_create_edge_node(tree, new_key, key_size, key_idx, key_len - key_idx, new_node);
                }

                radix_tree_refetch_edge(edge, tree, node);
                radix_tree_refetch_edge_str_unsafe(old_key, tree, edge, key_size);

                const struct RadixTreeNodePtr branch_node = radix_tree_create_branch_node(tree, RTT_BRANCH_4);
                struct RadixTreeBranch4Node* branch = radix_tree_fetch_branch4(tree, branch_node);

                branch->values[0] = old_val;
                branch->entries[0] = old_node;
                branch->values[1] = new_val;
                branch->entries[1] = new_node;

                // Remove old edge and potentially string
                radix_tree_edge_maybe_remove_str(tree, edge);
                freelist_remove(&tree->edges, node.idx);

                // Update previous reference with replacement branch node
                radix_tree_set_prev(tree, branch_node);
                return;
            }

            // Consume all matching characters.
            for (;;) {
                old_has_remainder = old_idx < edge->length;
                new_has_remainder = key_idx < key_len;

                // Keep incrementing as long as possible on both keys.
                if (!old_has_remainder || !new_has_remainder) {
                    break;
                }

                old_val = radix_tree_key_unpack(old_key, old_idx);
                new_val = radix_tree_key_unpack(new_key, key_idx);

                // Or until we find a mismatch.
                if (old_val != new_val) {
                    break;
                }

                key_idx++;
                old_idx++;
            }

            // old key has remainder + new key has remainder -> new branch index on val 1 and 2
            // one key has remainder + other does not -> branch with immediate value
            // both keys are exhausted -> leaf node (do nothing as we do not replace values)

            // Both keys have remainders.
            if (old_has_remainder && new_has_remainder) {
                struct RadixTreeNodePtr old_node = edge->next;
                struct RadixTreeNodePtr new_node = radix_tree_create_leaf_node(tree, value, value_size);

                if (edge->length - old_idx > 1) {
                    old_node = radix_tree_create_edge_node(
                        tree, old_key, key_size, old_idx + 1, edge->length - old_idx - 1, old_node
                    );
                }

                if (key_len - key_idx > 1) {
                    new_node = radix_tree_create_edge_node(
                        tree, new_key, key_size, key_idx + 1, key_len - key_idx - 1, new_node
                    );
                }

                radix_tree_refetch_edge(edge, tree, node);
                radix_tree_refetch_edge_str_unsafe(old_key, tree, edge, key_size);

                // Move string into local buffer if it's a small string
                radix_tree_edge_maybe_move_str(tree, edge, old_key, old_idx);

                edge->length = old_idx;
                edge->next = radix_tree_create_branch_node(tree, RTT_BRANCH_4);

                struct RadixTreeBranch4Node* branch = radix_tree_fetch_branch4(tree, edge->next);
                branch->values[0] = old_val;
                branch->entries[0] = old_node;
                branch->values[1] = new_val;
                branch->entries[1] = new_node;

                return;
            }

            // edge has remainder and needs to be split into a branch node
            // that gets our leaf as an immediate.
            if (old_has_remainder && !new_has_remainder) {
                const struct RadixTreeNodePtr new_branch_node = radix_tree_create_branch_node(tree, RTT_BRANCH_4);
                struct RadixTreeBranch4Node* new_branch = radix_tree_fetch_branch4(tree, new_branch_node);
                struct RadixTreeNodePtr new_node = radix_tree_create_leaf_node(tree, value, value_size);
                struct RadixTreeNodePtr old_node = edge->next;

                new_branch->values[0] = RADIX_BRANCH_IMMEDIATE;
                new_branch->entries[0] = new_node;

                radix_key_t branch_val = radix_tree_key_unpack(old_key, old_idx);

                if (edge->length - old_idx > 1) {
                    old_node = radix_tree_create_edge_node(
                        tree, old_key, key_size, old_idx + 1, edge->length - old_idx - 1, old_node
                    );

                    radix_tree_refetch_edge(edge, tree, node);
                    radix_tree_refetch_edge_str_unsafe(old_key, tree, edge, key_size);
                }

                new_branch->values[1] = branch_val;
                new_branch->entries[1] = old_node;

                // Move string into local buffer if it's a small string
                radix_tree_edge_maybe_move_str(tree, edge, old_key, old_idx);

                edge->length = old_idx;
                edge->next = new_branch_node;

                return;
            }

            // consumed entire old key, but we need to split the edge at the last value
            // and let the old one immediately point to the new leaf.
            if (!old_has_remainder && new_has_remainder && edge->next.type == RTT_LEAF) {
                const struct RadixTreeNodePtr new_branch_node = radix_tree_create_branch_node(tree, RTT_BRANCH_4);
                struct RadixTreeBranch4Node* new_branch = radix_tree_fetch_branch4(tree, new_branch_node);
                struct RadixTreeNodePtr new_node = radix_tree_create_leaf_node(tree, value, value_size);

                new_branch->values[0] = RADIX_BRANCH_IMMEDIATE;
                new_branch->entries[0] = edge->next;

                new_val = radix_tree_key_unpack(new_key, key_idx++);

                if (key_len - key_idx > 0) {
                    new_node =
                        radix_tree_create_edge_node(tree, new_key, key_size, key_idx, key_len - key_idx, new_node);

                    radix_tree_refetch_edge(edge, tree, node);
                }

                new_branch->values[1] = new_val;
                new_branch->entries[1] = new_node;
                edge->next = new_branch_node;

                return;
            }

            // If we have matched every single character in the edge key, the next node is not a leaf
            // and our own key is not exhausted we need to continue down the tree.
            radix_tree_push_prev(tree, node, RADIX_BRANCH_IMMEDIATE);
            node = edge->next;
        } break;
        case RTT_BRANCH_4:
        case RTT_BRANCH_8:
        case RTT_BRANCH_16:
        case RTT_BRANCH_FULL: {
            // If we hit a branch but have already consumed the entire key
            // we just need to insert the value at the immediate slot, no matter
            // what it contains. (We only insert leaves into the immediate slot)
            if (key_len == key_idx) {
                radix_tree_push_prev(tree, node, RADIX_BRANCH_IMMEDIATE);
                radix_tree_get_branch(tree, &node, RADIX_BRANCH_IMMEDIATE, &node);
                break;
            }

            const radix_key_t branch_val = radix_tree_key_unpack(new_key, key_idx++);

            const bool key_has_remainder = key_len > key_idx;

            struct RadixTreeNodePtr next;
            radix_tree_get_branch(tree, &node, branch_val, &next);

            // Handle special cases.

            switch (next.type) {
            case RTT_EDGE: {
                if (key_has_remainder) {
                    break;
                }

                // Special case 1: We have consumed the entire key but the next key is an edge
                // we need to replace it with a branch and set our value as its immediate value
                // while adding the edge to the branch by consuming one character from it.

                // replace edge with branch that takes next edge key value and this leaf as an
                // immediate value
                struct RadixTreeEdgeNode* edge = radix_tree_fetch_edge(tree, next);
                const radix_key_t* old_key = radix_tree_fetch_edge_str_unsafe(tree, edge, key_size);
                // first next edge key value
                const radix_key_t old_val = radix_tree_key_unpack(old_key, 0);

                struct RadixTreeNodePtr new_branch_node = radix_tree_create_branch_node(tree, RTT_BRANCH_4);

                // set this value as the immediate value of the new branch
                struct RadixTreeBranch4Node* new_branch = radix_tree_fetch_branch4(tree, new_branch_node);

                new_branch->values[0] = RADIX_BRANCH_IMMEDIATE;
                new_branch->entries[0] = radix_tree_create_leaf_node(tree, value, value_size);

                // simple case where we can just insert the edge next as the node for the edge.
                if (edge->length == 1) {
                    new_branch->values[1] = old_val;
                    new_branch->entries[1] = edge->next;

                    const struct RadixTreeNodePtr maybe_new =
                        radix_tree_insert_into_branch_or_grow(tree, node, branch_val, new_branch_node);

                    if (maybe_new.type != RTT_NONE) {
                        radix_tree_set_prev(tree, maybe_new);
                    }

                    freelist_remove(&tree->edges, next.idx);
                    return;
                }

                // we have to extract the first character of the edge key and create a new edge
                struct RadixTreeNodePtr new_edge_node =
                    radix_tree_create_edge_node(tree, old_key, key_size, 1, edge->length - 1, edge->next);

                new_branch->values[1] = old_val;
                new_branch->entries[1] = new_edge_node;

                radix_tree_refetch_edge(edge, tree, next);

                // Since we have to delete the old edge we need to clean up if it contains a string
                radix_tree_edge_maybe_remove_str(tree, edge);
                freelist_remove(&tree->edges, next.idx);

                const struct RadixTreeNodePtr maybe_new =
                    radix_tree_insert_into_branch_or_grow(tree, node, branch_val, new_branch_node);
                if (maybe_new.type != RTT_NONE) {
                    radix_tree_set_prev(tree, maybe_new);
                }
                return;
            } break;
            case RTT_BRANCH_4:
            case RTT_BRANCH_8:
            case RTT_BRANCH_16:
            case RTT_BRANCH_FULL: {
                if (key_has_remainder) {
                    break;
                }

                // Special case 2: We have consumed the entire key but the next node is a branch
                // we need to fill in the immediate field with our value.
                radix_tree_push_prev(tree, node, branch_val);
                radix_tree_push_prev(tree, next, RADIX_BRANCH_IMMEDIATE);
                radix_tree_get_branch(tree, &next, RADIX_BRANCH_IMMEDIATE, &node);
                // Break two levels to handle the next node.
                continue;
            } break;
            case RTT_LEAF: {
                if (!key_has_remainder) {
                    break;
                }

                // Special case 3: The next node from the branch is a leaf but our current key has a remainder
                // we have to branch and leave the old leaf as the immediate value while continuing with the new key.

                const struct RadixTreeNodePtr new_branch_node = radix_tree_create_branch_node(tree, RTT_BRANCH_4);

                struct RadixTreeBranch4Node* new_branch = radix_tree_fetch_branch4(tree, new_branch_node);
                new_branch->values[0] = RADIX_BRANCH_IMMEDIATE;
                new_branch->entries[0] = next;

                // insert new key into the new branch
                struct RadixTreeNodePtr new_node = radix_tree_create_leaf_node(tree, value, value_size);
                const radix_key_t new_key_val = radix_tree_key_unpack(new_key, key_idx++);

                // Take the rest of the key and insert it into a new edge node
                if (key_len - key_idx > 0) {
                    new_node =
                        radix_tree_create_edge_node(tree, new_key, key_size, key_idx, key_len - key_idx, new_node);
                }

                new_branch->values[1] = new_key_val;
                new_branch->entries[1] = new_node;

                const struct RadixTreeNodePtr maybe_new =
                    radix_tree_insert_into_branch_or_grow(tree, node, branch_val, new_branch_node);
                if (maybe_new.type != RTT_NONE) {
                    radix_tree_set_prev(tree, maybe_new);
                }

                return;
            } break;
            case RTT_NONE: {
            } break;
            default: {
                __builtin_unreachable();
            }
            }

            // If no special case is handled continue to the next node.
            radix_tree_push_prev(tree, node, branch_val);
            node = next;
        } break;
        case RTT_LEAF: {
            // replace value in leaf node.
            memcpy(freelist_get_unsafe(&tree->leaves, node.idx, value_size), value, value_size);
            return;
        } break;
        case RTT_NONE: {
            // Nothing more, insert value here.
            struct RadixTreeNodePtr new_node = radix_tree_create_leaf_node(tree, value, value_size);

            if (key_len - key_idx > 0) {
                new_node = radix_tree_create_edge_node(tree, new_key, key_size, key_idx, key_len - key_idx, new_node);
            }

            // Set previous reference to new node.
            radix_tree_set_prev(tree, new_node);
            return;
        } break;
        default:
            __builtin_unreachable();
        }
    }
}

void _radix_tree_get(
    const _RadixTreeBase* tree, const radix_key_t* key, const radix_key_idx_t key_len, const radix_key_idx_t key_size,
    void** value, const uint32_t value_size
) {
    struct RadixTreeNodePtr node = tree->root;
    radix_key_idx_t key_idx = 0;

    // We can take both a branch and a leaf even if we have no more
    // key characters to consume. But never an edge.
    for (;;) {
        switch (node.type) {
        case RTT_EDGE: {
            const struct RadixTreeEdgeNode* edge = radix_tree_fetch_edge(tree, node);
            const radix_key_t* edge_key = radix_tree_fetch_edge_str_unsafe(tree, edge, key_size);

            if (edge->length > key_len - key_idx) {
                goto notfound;
            }

            radix_key_idx_t old_idx = edge->length & ~1;

            const radix_key_idx_t byte_len = old_idx / RADIX_TREE_KEY_RATIO;

            radix_key_t aligned_key[byte_len];
            radix_tree_copy_key(aligned_key, key, key_idx, old_idx);

            if (memcmp(edge_key, aligned_key, byte_len) != 0) {
                goto notfound;
            }

            key_idx += old_idx;

            for (; old_idx < edge->length; old_idx++) {
                if (radix_tree_key_unpack(edge_key, old_idx) != radix_tree_key_unpack(key, key_idx++)) {
                    goto notfound;
                }
            }

            node = edge->next;
        } break;
        case RTT_BRANCH_4: {
            const struct RadixTreeBranch4Node* branch = radix_tree_fetch_branch4(tree, node);

            const radix_key_t val = key_len <= key_idx ? RADIX_BRANCH_IMMEDIATE : radix_tree_key_unpack(key, key_idx++);

            radix_tree_get_branch4(branch, val, &node);
        } break;
        case RTT_BRANCH_8: {
            const struct RadixTreeBranch8Node* branch = radix_tree_fetch_branch8(tree, node);
            const radix_key_t val = key_len <= key_idx ? RADIX_BRANCH_IMMEDIATE : radix_tree_key_unpack(key, key_idx++);
            radix_tree_get_branch8(branch, val, &node);
        } break;
        case RTT_BRANCH_16: {
            const struct RadixTreeBranch16Node* branch = radix_tree_fetch_branch16(tree, node);
            radix_tree_get_branch_16(branch, radix_tree_key_unpack(key, key_idx++), &node);
        } break;
        case RTT_BRANCH_FULL: {
            const struct RadixTreeBranchFullNode* branch = radix_tree_fetch_branch_full(tree, node);
            const radix_key_t val = key_len <= key_idx ? RADIX_BRANCH_IMMEDIATE : radix_tree_key_unpack(key, key_idx++);
            radix_tree_get_branch_full(branch, val, &node);
        } break;
        case RTT_LEAF: {
            *value = radix_tree_fetch_leaf_unsafe(tree, node, value_size);
            return;
        } break;
        default:
            goto notfound;
        }
    }

notfound:
    *value = NULL;
}

void radix_tree_debug_node(
    _RadixTreeBase* tree, const struct RadixTreeNodePtr node, FILE* stream, const uint32_t indent,
    const uint32_t key_size, const uint32_t value_size
) {
    switch (node.type) {
    case RTT_BRANCH_4: {
        const struct RadixTreeBranch4Node* branch = radix_tree_fetch_branch4(tree, node);
        for (uint32_t i = 0; i < 4; i++) {
            if (branch->entries[i].type == RTT_NONE) {
                continue;
            }
            if (branch->values[i] == RADIX_BRANCH_IMMEDIATE) {
                fprintf(stream, "%*s[Immediate]\n", indent, "");
            } else {
                fprintf(stream, "%*s[%01x]\n", indent, "", branch->values[i]);
            }
            radix_tree_debug_node(tree, branch->entries[i], stream, indent + 2, key_size, value_size);
        }
    } break;
    case RTT_BRANCH_8: {
        const struct RadixTreeBranch8Node* branch = radix_tree_fetch_branch8(tree, node);
        for (uint32_t i = 0; i < 8; i++) {
            if (branch->entries[i].type == RTT_NONE) {
                continue;
            }

            if (branch->values[i] == RADIX_BRANCH_IMMEDIATE) {
                fprintf(stream, "%*s[Immediate]\n", indent, "");
            } else {
                fprintf(stream, "%*s[%01x]\n", indent, "", branch->values[i]);
            }

            radix_tree_debug_node(tree, branch->entries[i], stream, indent + 2, key_size, value_size);
        }
    } break;
    case RTT_BRANCH_16: {
        const struct RadixTreeBranch16Node* branch = radix_tree_fetch_branch16(tree, node);
        for (uint32_t i = 0; i < RADIX_TREE_KEY_BRANCHES; i++) {
            if (branch->entries[i].type == RTT_NONE) {
                continue;
            }
            fprintf(stream, "%*s[%01x]\n", indent, "", i);
            radix_tree_debug_node(tree, branch->entries[i], stream, indent + 2, key_size, value_size);
        }
    } break;
    case RTT_BRANCH_FULL: {
        const struct RadixTreeBranchFullNode* branch = radix_tree_fetch_branch_full(tree, node);
        for (uint32_t i = 0; i < RADIX_TREE_KEY_BRANCHES + 1; i++) {
            if (branch->entries[i].type == RTT_NONE) {
                continue;
            }
            if (i == RADIX_BRANCH_IMMEDIATE) {
                fprintf(stream, "%*s[Immediate]\n", indent, "");
            } else {
                fprintf(stream, "%*s[%01x]\n", indent, "", i);
            }
            radix_tree_debug_node(tree, branch->entries[i], stream, indent + 2, key_size, value_size);
        }
    } break;
    case RTT_EDGE: {
        const struct RadixTreeEdgeNode* edge = radix_tree_fetch_edge(tree, node);
        fprintf(stream, "%*sEdge\n", indent, "");
        fprintf(stream, "%*sLength: %d\n", indent + 2, "", edge->length);
        fprintf(stream, "%*sData: ", indent + 2, "");
        const radix_key_t* edge_str = radix_tree_fetch_edge_str_unsafe(tree, edge, key_size);
        for (uint32_t i = 0; i < edge->length; i++) {
            fprintf(stream, "%01x", radix_tree_key_unpack(edge_str, i));
        }
        fprintf(stream, "\n");
        if (edge->next.type == RTT_NONE) {
            return;
        }
        radix_tree_debug_node(tree, edge->next, stream, indent + 2, key_size, value_size);
    } break;
    case RTT_LEAF: {
        const void* leaf = radix_tree_fetch_leaf_unsafe(tree, node, value_size);
        fprintf(stream, "%*sLeaf\n", indent, "");
        fprintf(stream, "%*sValue: %p\n", indent + 2, "", leaf);
    } break;
    case RTT_NONE: {
        fprintf(stream, "%*sNone\n", indent, "");
    } break;
    default:
        __builtin_unreachable();
    }
}