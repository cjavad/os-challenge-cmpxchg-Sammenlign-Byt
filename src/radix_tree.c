#include "radix_tree.h"

void _radix_tree_insert(
    _RadixTreeBase* tree, const radix_key_t* new_key, const radix_key_idx_t key_len, const radix_key_idx_t key_size,
    const void* value, const uint32_t value_size
) {
    struct RadixTreeNodePtr node = tree->root;
    struct RadixTreeNodePtr prev = {.type = RTT_NONE};

    radix_key_idx_t key_idx = 0;
    radix_key_t prev_branch_val = RADIX_BRANCH_IMMEDIATE;

    for (;;) {
    handle_next_node:
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

                const struct RadixTreeNodePtr branch_node = radix_tree_create_branch_node(tree);
                struct RadixTreeBranchNode* branch = radix_tree_fetch_branch(tree, branch_node);

                branch->next[old_val] = old_node;
                branch->next[new_val] = new_node;

                // Remove old edge and potentially string
                if (edge->length > RADIX_TREE_SMALL_STR_SIZE) {
                    freelist_remove(&tree->strings, edge->string_idx);
                }

                freelist_remove(&tree->edges, node.idx);

                // Update previous reference with replacement branch node
                radix_tree_set_prev(tree, prev, prev_branch_val, branch_node);
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
                if (edge->length > RADIX_TREE_SMALL_STR_SIZE && old_idx <= RADIX_TREE_SMALL_STR_SIZE) {
                    const uint32_t old_str_idx = edge->string_idx;
                    radix_tree_copy_key(edge->data, old_key, 0, old_idx);
                    freelist_remove(&tree->strings, old_str_idx);
                }

                edge->length = old_idx;
                edge->next = radix_tree_create_branch_node(tree);

                struct RadixTreeBranchNode* branch = radix_tree_fetch_branch(tree, edge->next);

                branch->next[old_val] = old_node;
                branch->next[new_val] = new_node;

                return;
            }

            // edge has remainder and needs to be split into a branch node
            // that gets our leaf as an immediate.
            if (old_has_remainder && !new_has_remainder) {
                const struct RadixTreeNodePtr new_branch_node = radix_tree_create_branch_node(tree);
                struct RadixTreeBranchNode* new_branch = radix_tree_fetch_branch(tree, new_branch_node);
                struct RadixTreeNodePtr new_node = radix_tree_create_leaf_node(tree, value, value_size);
                struct RadixTreeNodePtr old_node = edge->next;

                new_branch->next[RADIX_BRANCH_IMMEDIATE] = new_node;
                radix_key_t branch_val = radix_tree_key_unpack(old_key, old_idx);

                if (edge->length - old_idx > 1) {
                    old_node = radix_tree_create_edge_node(
                        tree, old_key, key_size, old_idx + 1, edge->length - old_idx - 1, old_node
                    );

                    radix_tree_refetch_edge(edge, tree, node);
                    radix_tree_refetch_edge_str_unsafe(old_key, tree, edge, key_size);
                }

                new_branch->next[branch_val] = old_node;

                // Move string into local buffer if it's a small string
                if (edge->length > RADIX_TREE_SMALL_STR_SIZE && old_idx <= RADIX_TREE_SMALL_STR_SIZE) {
                    const uint32_t old_str_idx = edge->string_idx;
                    radix_tree_copy_key(edge->data, old_key, 0, old_idx);
                    freelist_remove(&tree->strings, old_str_idx);
                }
                edge->length = old_idx;
                edge->next = new_branch_node;

                return;
            }

            // consumed entire old key, but we need to split the edge at the last value
            // and let the old one immediately point to the new leaf.
            if (!old_has_remainder && new_has_remainder && edge->next.type == RTT_LEAF) {
                const struct RadixTreeNodePtr new_branch_node = radix_tree_create_branch_node(tree);
                struct RadixTreeBranchNode* new_branch = radix_tree_fetch_branch(tree, new_branch_node);
                struct RadixTreeNodePtr new_node = radix_tree_create_leaf_node(tree, value, value_size);

                new_branch->next[RADIX_BRANCH_IMMEDIATE] = edge->next;

                new_val = radix_tree_key_unpack(new_key, key_idx++);

                if (key_len - key_idx > 0) {
                    new_node =
                        radix_tree_create_edge_node(tree, new_key, key_size, key_idx, key_len - key_idx, new_node);

                    radix_tree_refetch_edge(edge, tree, node);
                }

                new_branch->next[new_val] = new_node;
                edge->next = new_branch_node;

                return;
            }

            // If we have matched every single character in the edge key, the next node is not a leaf
            // and our own key is not exhausted we need to continue down the tree.
            prev = node;
            node = edge->next;
        } break;
        case RTT_BRANCH: {
            struct RadixTreeBranchNode* branch = radix_tree_fetch_branch(tree, node);

            // If we hit a branch but have already consumed the entire key
            // we just need to insert the value at the immediate slot, no matter
            // what it contains. (We only insert leaves into the immediate slot)
            if (key_len == key_idx) {
                prev_branch_val = RADIX_BRANCH_IMMEDIATE;
                prev = node;
                node = branch->next[prev_branch_val];
                break;
            }

            prev_branch_val = radix_tree_key_unpack(new_key, key_idx++);
            const bool key_has_remainder = key_len > key_idx;
            const struct RadixTreeNodePtr next = branch->next[prev_branch_val];

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

                struct RadixTreeNodePtr new_branch_node = radix_tree_create_branch_node(tree);
                radix_tree_refetch_branch(branch, tree, node);

                // set this value as the immediate value of the new branch
                struct RadixTreeBranchNode* new_branch = radix_tree_fetch_branch(tree, new_branch_node);
                new_branch->next[RADIX_BRANCH_IMMEDIATE] = radix_tree_create_leaf_node(tree, value, value_size);

                // simple case where we can just insert the edge next as the node for the edge.
                if (edge->length == 1) {
                    new_branch->next[old_val] = edge->next;
                    branch->next[prev_branch_val] = new_branch_node;
                    freelist_remove(&tree->edges, next.idx);
                    return;
                }

                // we have to extract the first character of the edge key and create a new edge
                struct RadixTreeNodePtr new_edge_node =
                    radix_tree_create_edge_node(tree, old_key, key_size, 1, edge->length - 1, edge->next);

                new_branch->next[old_val] = new_edge_node;

                radix_tree_refetch_edge(edge, tree, next);

                // Since we have to delete the old edge we need to clean up if it contains a string
                if (edge->length > RADIX_TREE_SMALL_STR_SIZE) {
                    freelist_remove(&tree->strings, edge->string_idx);
                }

                freelist_remove(&tree->edges, next.idx);

                branch->next[prev_branch_val] = new_branch_node;

                return;
            } break;
            case RTT_BRANCH: {
                if (key_has_remainder) {
                    break;
                }

                // Special case 2: We have consumed the entire key but the next node is a branch
                // we need to fill in the immediate field with our value.

                const struct RadixTreeBranchNode* next_branch = radix_tree_fetch_branch(tree, next);

                prev = next;
                prev_branch_val = RADIX_BRANCH_IMMEDIATE;
                node = next_branch->next[prev_branch_val];

                // Break two levels to handle the next node.
                goto handle_next_node;
            } break;
            case RTT_LEAF: {
                if (!key_has_remainder) {
                    break;
                }

                // Special case 3: The next node from the branch is a leaf but our current key has a remainder
                // we have to branch and leave the old leaf as the immediate value while continuing with the new key.

                const struct RadixTreeNodePtr new_branch_node = radix_tree_create_branch_node(tree);
                radix_tree_refetch_branch(branch, tree, node);

                struct RadixTreeBranchNode* new_branch = radix_tree_fetch_branch(tree, new_branch_node);
                new_branch->next[RADIX_BRANCH_IMMEDIATE] = next;

                // insert new key into the new branch
                struct RadixTreeNodePtr new_node = radix_tree_create_leaf_node(tree, value, value_size);
                const radix_key_t new_key_val = radix_tree_key_unpack(new_key, key_idx++);

                // Take the rest of the key and insert it into a new edge node
                if (key_len - key_idx > 0) {
                    new_node =
                        radix_tree_create_edge_node(tree, new_key, key_size, key_idx, key_len - key_idx, new_node);
                }

                new_branch->next[new_key_val] = new_node;

                branch->next[prev_branch_val] = new_branch_node;

                return;
            } break;
            case RTT_NONE: {
                // We go to this node.
            } break;
            default: {
                __builtin_unreachable();
            }
            }

            // If no special case is handled continue to the next node.
            prev = node;
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
            radix_tree_set_prev(tree, prev, prev_branch_val, new_node);
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

            for (radix_key_idx_t i = 0; i < edge->length; i++) {
                if (radix_tree_key_unpack(edge_key, i) == radix_tree_key_unpack(key, key_idx++)) {
                    continue;
                }

                goto notfound;
            }

            node = edge->next;
        } break;
        case RTT_BRANCH: {
            const struct RadixTreeBranchNode* branch = radix_tree_fetch_branch(tree, node);
            radix_key_t val;

            if (key_len <= key_idx) {
                // If the key is exhausted, we have an exact match either return the immediate value or
                // no match.
                val = RADIX_BRANCH_IMMEDIATE;
            } else {
                val = radix_tree_key_unpack(key, key_idx++);
            }

            // Always take branch, if it is none we hit the default case.
            node = branch->next[val];

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
    case RTT_BRANCH: {
        const struct RadixTreeBranchNode* branch = radix_tree_fetch_branch(tree, node);
        if (branch->next[RADIX_BRANCH_IMMEDIATE].type != RTT_NONE) {
            fprintf(stream, "%*s[Immediate]\n", indent + 2, "");
            radix_tree_debug_node(tree, branch->next[RADIX_BRANCH_IMMEDIATE], stream, indent + 2, key_size, value_size);
        }
        for (uint32_t i = 0; i < RADIX_TREE_KEY_BRANCHES; i++) {
            if (branch->next[i].type != RTT_NONE) {
                fprintf(stream, "%*s[%x]\n", indent + 2, "", i);
                radix_tree_debug_node(tree, branch->next[i], stream, indent + 2, key_size, value_size);
            }
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