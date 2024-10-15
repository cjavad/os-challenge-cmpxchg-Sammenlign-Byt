#include "radix_tree.h"

void _radix_tree_insert(
    _RadixTreeBase* tree, const radix_key_t* new_key, const radix_key_idx_t key_length, const void* value,
    const uint32_t value_size
) {
    struct RadixTreeNodePtr node = tree->root;
    struct RadixTreeNodePtr prev = {.type = RTT_NONE};

    radix_key_idx_t key_idx = 0;
    radix_key_idx_t prev_branch_val = 0;

    if (node.type == RTT_NONE) {
        const struct RadixTreeNodePtr leaf = radix_tree_create_leaf_node(tree, value, value_size);
        tree->root = radix_tree_create_edge_node(tree, new_key, key_length, 0, key_length, leaf);
        return;
    }

    while (key_idx < key_length) {
        switch (node.type) {
        case RTT_EDGE: {
            struct RadixTreeEdgeNode* edge = radix_tree_fetch_edge(tree, node);
            const radix_key_t* old_key = radix_tree_fetch_edge_str_unsafe(tree, edge, key_length);

            radix_key_t old_val = radix_tree_key_unpack(old_key, 0);
            radix_key_t new_val = radix_tree_key_unpack(new_key, key_idx++);

            if (old_val != new_val) {
                struct RadixTreeNodePtr old_node = edge->next;
                struct RadixTreeNodePtr new_node = radix_tree_create_leaf_node(tree, value, value_size);

                if (edge->length > 1) {
                    old_node = radix_tree_create_edge_node(tree, old_key, key_length, 1, edge->length - 1, old_node);
                }

                if (key_length - key_idx > 0) {
                    new_node =
                        radix_tree_create_edge_node(tree, new_key, key_length, key_idx, key_length - key_idx, new_node);
                }

                radix_tree_refetch_edge(edge, tree, node);
                radix_tree_refetch_edge_str_unsafe(old_key, tree, edge, key_length);

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

            for (radix_key_idx_t i = 1; i < edge->length; i++) {
                old_val = radix_tree_key_unpack(old_key, i);
                new_val = radix_tree_key_unpack(new_key, key_idx);

                // Consume edge key until it differs from new key, or we reach the end of the new key
                // (meaning we have an exact match that needs to branch here)
                if (old_val == new_val && key_idx < key_length) {
                    key_idx++;
                    continue;
                }

                struct RadixTreeNodePtr old_node = edge->next;
                struct RadixTreeNodePtr new_node = radix_tree_create_leaf_node(tree, value, value_size);

                if (edge->length - i > 1) {
                    old_node =
                        radix_tree_create_edge_node(tree, old_key, key_length, i + 1, edge->length - i - 1, old_node);
                }

                if (key_length - key_idx > 0) {
                    new_node = radix_tree_create_edge_node(
                        tree, new_key, key_length, key_idx + 1, key_length - key_idx - 1, new_node
                    );
                }

                radix_tree_refetch_edge(edge, tree, node);
                radix_tree_refetch_edge_str_unsafe(old_key, tree, edge, key_length);

                // Move string into local buffer if it's a small string
                if (edge->length > RADIX_TREE_SMALL_STR_SIZE && i <= RADIX_TREE_SMALL_STR_SIZE) {
                    const uint32_t old_str_idx = edge->string_idx;
                    radix_tree_copy_key(edge->data, old_key, 0, i);
                    freelist_remove(&tree->strings, old_str_idx);
                }

                edge->length = i;
                edge->next = radix_tree_create_branch_node(tree);

                struct RadixTreeBranchNode* branch = radix_tree_fetch_branch(tree, edge->next);

                branch->next[old_val] = old_node;
                branch->next[new_val] = new_node;

                return;
            }

            prev = node;
            node = edge->next;
        } break;
        case RTT_BRANCH: {
            struct RadixTreeBranchNode* branch = radix_tree_fetch_branch(tree, node);
            prev_branch_val = radix_tree_key_unpack(new_key, key_idx++);
            const struct RadixTreeNodePtr next = branch->next[prev_branch_val];

            if (next.type == RTT_NONE) {
                struct RadixTreeNodePtr new_node = radix_tree_create_leaf_node(tree, value, value_size);

                if (key_length - key_idx > 0) {
                    new_node =
                        radix_tree_create_edge_node(tree, new_key, key_length, key_idx, key_length - key_idx, new_node);
                }

                branch->next[prev_branch_val] = new_node;
                return;
            }

            prev = node;
            node = next;
        } break;
        default:
            return;
        }
    }
}

void _radix_tree_get(
    const _RadixTreeBase* tree, const radix_key_t* key, const radix_key_idx_t key_length, void* value,
    const uint32_t value_size
) {
    struct RadixTreeNodePtr node = tree->root;
    radix_key_idx_t key_idx = 0;

    while (key_idx < key_length || node.type == RTT_LEAF) {
        switch (node.type) {
        case RTT_EDGE: {
            const struct RadixTreeEdgeNode* edge = radix_tree_fetch_edge(tree, node);
            const radix_key_t* edge_key = radix_tree_fetch_edge_str_unsafe(tree, edge, key_length);

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
            // Always take branch, if it is none we hit the default case.
            node = branch->next[radix_tree_key_unpack(key, key_idx++)];
        } break;
        case RTT_LEAF: {
            memcpy(value, radix_tree_fetch_leaf_unsafe(tree, node, value_size), value_size);
            return;
        } break;
        default:
            goto notfound;
        }
    }

notfound:
    memset(value, 0, value_size);
}

void radix_tree_debug_node(
    _RadixTreeBase* tree, const struct RadixTreeNodePtr node, FILE* stream, const uint32_t indent,
    const uint32_t key_length, const uint32_t value_size
) {
    switch (node.type) {
    case RTT_BRANCH: {
        const struct RadixTreeBranchNode* branch = radix_tree_fetch_branch(tree, node);
        for (uint32_t i = 0; i < RADIX_TREE_KEY_BRANCHES; i++) {
            if (branch->next[i].type != RTT_NONE) {
                fprintf(stream, "%*s[%x]\n", indent + 2, "", i);
                radix_tree_debug_node(tree, branch->next[i], stream, indent + 2, key_length, value_size);
            }
        }
    } break;
    case RTT_EDGE: {
        const struct RadixTreeEdgeNode* edge = radix_tree_fetch_edge(tree, node);
        fprintf(stream, "%*sEdge\n", indent, "");
        fprintf(stream, "%*sLength: %d\n", indent + 2, "", edge->length);
        fprintf(stream, "%*sData: ", indent + 2, "");
        const radix_key_t* edge_str = radix_tree_fetch_edge_str_unsafe(tree, edge, key_length);
        for (uint32_t i = 0; i < edge->length; i++) {
            fprintf(stream, "%01x", radix_tree_key_unpack(edge_str, i));
        }
        fprintf(stream, "\n");
        if (edge->next.type == RTT_NONE) {
            return;
        }
        radix_tree_debug_node(tree, edge->next, stream, indent + 2, key_length, value_size);
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