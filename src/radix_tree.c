#include "radix_tree.h"

void _radix_tree_insert(
    _RadixTreeBase* tree, const radix_key_t* new_key, const radix_key_idx_t key_len, const radix_key_idx_t key_size,
    const void* value, const uint32_t value_size
) {
    struct RadixTreeNodePtr node = tree->root;
    struct RadixTreeNodePtr prev = {.type = RTT_NONE};

    radix_key_idx_t key_idx = 0;
    radix_key_idx_t prev_branch_val = 0;

    if (node.type == RTT_NONE) {
        const struct RadixTreeNodePtr leaf = radix_tree_create_leaf_node(tree, value, value_size);
        tree->root = radix_tree_create_edge_node(tree, new_key, key_size, 0, key_len, leaf);
        return;
    }

    while (key_idx < key_len) {
        switch (node.type) {
        case RTT_EDGE: {
            struct RadixTreeEdgeNode* edge = radix_tree_fetch_edge(tree, node);
            const radix_key_t* old_key = radix_tree_fetch_edge_str_unsafe(tree, edge, key_size);

            radix_key_t old_val = radix_tree_key_unpack(old_key, 0);
            radix_key_t new_val = radix_tree_key_unpack(new_key, key_idx++);

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
                switch (prev.type) {
                case RTT_EDGE: {
                    radix_tree_fetch_edge(tree, prev)->next = branch_node;
                } break;
                case RTT_BRANCH: {
                    radix_tree_fetch_branch(tree, prev)->next[prev_branch_val] = branch_node;
                } break;
                default: {
                    tree->root = branch_node;
                } break;
                }

                return;
            }

            // Consume all matching characters.
            radix_key_idx_t old_idx = 1;

            // Keep incrementing as long as possible on both keys.
            while (old_idx < edge->length && key_idx < key_len &&
                   (old_val = radix_tree_key_unpack(old_key, old_idx)) ==
                       (new_val = radix_tree_key_unpack(new_key, key_idx))) {
                key_idx++;
                old_idx++;
            }

            // old key has remainder + new key has remainder -> new branch index on val 1 and 2
            // one key has remainder + other does not -> branch with immediate value
            // both keys are exhausted -> leaf node (do nothing as we do not replace values)

            const bool old_has_remainder = old_idx < edge->length;
            const bool new_has_remainder = key_idx < key_len;

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

                new_branch->immediate = new_node;
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

                new_branch->immediate = edge->next;

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

            // Both keys fully match and are exhausted but the edge splits into a branch.
            // then add the value to the immediate of the branch if not set.
            if (!old_has_remainder && !new_has_remainder && edge->next.type == RTT_BRANCH) {
                struct RadixTreeBranchNode* branch = radix_tree_fetch_branch(tree, edge->next);
                if (branch->immediate.type == RTT_NONE) {
                    branch->immediate = radix_tree_create_leaf_node(tree, value, value_size);
                }
                return;
            }

            // If we have matched every single character in the edge key, the next node is not a leaf
            // and our own key is not exhausted we need to continue down the tree.
            prev = node;
            node = edge->next;
        } break;
        case RTT_BRANCH: {
            struct RadixTreeBranchNode* branch = radix_tree_fetch_branch(tree, node);
            prev_branch_val = radix_tree_key_unpack(new_key, key_idx++);
            const struct RadixTreeNodePtr next = branch->next[prev_branch_val];

            // If the branch slot is empty populate it with a new leaf node
            if (next.type == RTT_NONE) {
                struct RadixTreeNodePtr new_node = radix_tree_create_leaf_node(tree, value, value_size);

                if (key_len - key_idx > 0) {
                    new_node =
                        radix_tree_create_edge_node(tree, new_key, key_size, key_idx, key_len - key_idx, new_node);
                }

                branch->next[prev_branch_val] = new_node;
                return;
            }

            // If the branch slot is taken but the key is exhausted, we have an exact match
            // and fill out
            if (key_len - key_idx == 0) {
                switch (next.type) {
                case RTT_EDGE: {
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
                    new_branch->immediate = radix_tree_create_leaf_node(tree, value, value_size);

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
                } break;
                case RTT_BRANCH: {
                    // set branch immediate if not set
                    struct RadixTreeBranchNode* next_branch = radix_tree_fetch_branch(tree, next);
                    // If the branch is empty, we can replace it with a leaf node.
                    if (next_branch->immediate.type == RTT_NONE) {
                        next_branch->immediate = radix_tree_create_leaf_node(tree, value, value_size);
                    }
                    // We do not replace existing values.
                } break;
                default: {
                    // if it already is a leaf node we can't do anything
                    // as we do not replace values.
                } break;
                }

                return;
            }

            // If the next node is a leaf (not an edge) we must manually set it to the immediate value of a new branch.
            // We first have to deal with if the current key is already empty, but if we have at least 1 character
            // we can continue here.
            if (next.type == RTT_LEAF) {
                const struct RadixTreeNodePtr new_branch_node = radix_tree_create_branch_node(tree);
                radix_tree_refetch_branch(branch, tree, node);

                struct RadixTreeBranchNode* new_branch = radix_tree_fetch_branch(tree, new_branch_node);
                new_branch->immediate = next;

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
            }

            prev = node;
            node = next;
        } break;
        default:
            // We do not replace values.
            return;
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
    while (key_idx < key_len || node.type != RTT_EDGE) {
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

            // If the key is exhausted, we have an exact match either return the immediate value or
            // no match.
            if (key_len == key_idx) {
                if (branch->immediate.type != RTT_NONE) {
                    node = branch->immediate;
                    break;
                }

                goto notfound;
            }

            // Always take branch, if it is none we hit the default case.
            node = branch->next[radix_tree_key_unpack(key, key_idx++)];
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
        if (branch->immediate.type != RTT_NONE) {
            fprintf(stream, "%*s[Immediate]\n", indent + 2, "");
            radix_tree_debug_node(tree, branch->immediate, stream, indent + 2, key_size, value_size);
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
        break;
    }
}