/*
-------------------------------------------------------
Name of project: Translator IFJ26
Authors: xsvoboj01 (Jakub Svoboda)
Creation date: 19.10.2026
File: symtable.h

Description: Implementation file for symbol table as a stack of height-balanced AVL trees
-------------------------------------------------------
*/

#include "symbol_table.h"

bool symtable_init(symtable_stack_t *stack) {
    if (stack == NULL) {
        return false;
    }
    stack->top = NULL;
    stack->global = NULL;
    return true;
}

bool symtable_create_avl(symtable_stack_t *stack) {
    if (stack == NULL) {
        return false;
    }

    // Allocation of new item of symtable
    symtable_stack_item_t *new_item =
        (symtable_stack_item_t *)malloc(sizeof(symtable_stack_item_t));
    if (new_item == NULL) {
        return false;
    }

    new_item->tree_root = NULL;

    new_item->next = stack->top;

    // Assign global scope if it's first symtable
    if (stack->global == NULL) {
        stack->global = new_item;
    }

    // Assign new top of stack
    stack->top = new_item;

    return true;
}

// Get higher number from two
int max(int left, int right) {
    if (left < right) {
        return right;
    }

    return left;
}

// Returns height of subtree, 0 if empty
int get_height(symtable_node_t *node) {
    if (node == NULL) {
        return 0;
    }
    return node->height;
}

// Right rotation
symtable_node_t *rotate_right_ll(symtable_node_t *root) {
    symtable_node_t *l_child = root->left;
    symtable_node_t *l_child_r_subtree = root->left->right;

    // Swap nodes
    l_child->right = root;
    root->left = l_child_r_subtree;

    // Get heights of children
    int left_h = get_height(root->right);
    int right_h = get_height(root->left);

    // Assign new heights
    root->height = max(left_h, right_h) + 1;

    // Calculate new heights
    left_h = get_height(l_child->left);
    right_h = get_height(l_child->right);

    l_child->height = max(left_h, right_h) + 1;

    return l_child;
}

// Left rotation
symtable_node_t *rotate_left_rr(symtable_node_t *root) {
    symtable_node_t *r_child = root->right;
    symtable_node_t *r_child_l_subtree = root->right->left;

    // Swap nodes
    r_child->left = root;
    root->right = r_child_l_subtree;

    // Get heights of children
    int left_h = get_height(root->right);
    int right_h = get_height(root->left);

    // Assign new heights
    root->height = max(left_h, right_h) + 1;

    // Calculate new heights
    left_h = get_height(r_child->left);
    right_h = get_height(r_child->right);

    r_child->height = max(left_h, right_h) + 1;

    return r_child;
}

// Left-right rotation
symtable_node_t *rotate_left_right_lr(symtable_node_t *root) {
    // Rotate left child first
    root->left = rotate_left_rr(root->left);
    // Then rotate the node
    return rotate_right_ll(root);
}

// Right-left rotation
symtable_node_t *rotate_right_left_rl(symtable_node_t *root) {
    // Rotate right child first
    root->right = rotate_right_ll(root->right);
    // Then rotate the node
    return rotate_left_rr(root);
}

symtable_node_t *insert_node(symtable_node_t *root, symtable_type_t type, char *id,
                             symtable_data_t data, bool *success) {
    if (root == NULL) {
        // Allocation of memory for the node
        symtable_node_t *node = (symtable_node_t *)malloc(sizeof(symtable_node_t));
        if (node == NULL) {
            *success = false;
            return NULL;
        }

        node->key = (char *)malloc(sizeof(char) * (strlen(id) + 1));
        if (node->key == NULL) {
            free(node);
            *success = false;
            return NULL;
        }

        // Initialization of node values
        strcpy(node->key, id);
        node->data = data;
        node->height = 1;
        node->left = NULL;
        node->right = NULL;

        return node;
    }

    int cmp = strcmp(root->key, id);

    // Decide which side to follow
    if (cmp > 0) {
        root->left = insert_node(root->left, type, id, data, success);
    } else if (cmp < 0) {
        root->right = insert_node(root->right, type, id, data, success);
    } else {
        *success = false;
    }

    // Get height of left and right subtrees
    int left_h = get_height(root->left);
    int right_h = get_height(root->right);

    // Update height
    root->height = max(left_h, right_h) + 1;

    // Calculate balance factor of current node
    int balance = left_h - right_h;

    // Decide which method of rotation to use
    if (balance >= 2) {
        // Calculate balance for left child
        balance = get_height(root->left->left) - get_height(root->left->right);
        // Decide method
        if (balance >= 0) {
            // LL
            return rotate_right_ll(root);
        } else {
            // LR
            return rotate_left_right_lr(root);
        }
    } else if (balance <= -2) {
        // Calculate balance for right child
        balance = get_height(root->right->left) - get_height(root->right->right);
        // Decide method
        if (balance <= 0) {
            // RR
            return rotate_left_rr(root);
        } else {
            // RL
            return rotate_right_left_rl(root);
        }
    }

    // Return root without change
    return root;
}

bool symtable_insert_item(symtable_stack_t *stack, symtable_type_t type, char *id,
                          symtable_data_t data) {
    if (stack == NULL || id == NULL) {
        return false;
    }

    bool success = true;

    stack->top->tree_root = insert_node(stack->top->tree_root, type, id, data, &success);

    return success;
}

symtable_node_t *symtable_stack_search_current(symtable_stack_t *stack, symtable_type_t type,
                                               char *id) {
    if (stack == NULL || id == NULL) {
        return NULL;
    }
    if (stack->top == NULL) {
        return NULL;
    }

    symtable_node_t *root = stack->top->tree_root;
    if (root == NULL) {
        return NULL;
    }

    // Decide which side to follow
    int cmp = strcmp(root->key, id);
    while (cmp != 0 && type != root->data.type) {
        if (cmp > 0) {
            root = root->left;
        } else {
            root = root->right;
        }

        if (root == NULL) {
            return NULL;
        }

        cmp = strcmp(root->key, id);
    }

    return root;
}

symtable_node_t *symtable_search(symtable_stack_t *stack, symtable_type_t type, char *id) {
    if (stack == NULL || id == NULL) {
        return NULL;
    }

    symtable_stack_item_t *top = stack->top;
    if (top == NULL) {
        return NULL;
    }

    symtable_node_t *node = NULL;
    while (top != NULL) {
        symtable_node_t *root = top->tree_root;
        if (root == NULL) {
            return NULL;
        }

        // Decide which side to follow
        int cmp = strcmp(root->key, id);
        while (cmp != 0 && type != root->data.type) {
            if (cmp > 0) {
                root = root->left;
            } else {
                root = root->right;
            }

            if (root == NULL) {
                return NULL;
            }

            cmp = strcmp(root->key, id);
        }
        if (node != NULL) {
            return node;
        }

        top = top->next;
    }

    return NULL;
}

// Recursively delete node and its children
void delete_node(symtable_node_t *node) {
    if (node == NULL) {
        return;
    }

    delete_node(node->left);
    delete_node(node->right);

    free(node->key);

    // Delete function parameters and its names
    if (node->data.type == SYM_TYPE_FUNC) {
        symtable_fun_param_t *param = node->data.fun.params;
        symtable_fun_param_t *next_param;
        while (param != NULL) {
            next_param = param->next;
            free(param->name);
            free(param);
            param = next_param;
        }
    }

    free(node);
}

void symtable_pop_avl(symtable_stack_t *stack) {
    if (stack == NULL) {
        return;
    }
    if (stack->top == NULL) {
        return;
    }

    delete_node(stack->top->tree_root);
}

void symtable_stack_delete(symtable_stack_t *stack) {
    if (stack == NULL) {
        return;
    }

    // Delete each AVL tree
    while (stack->top != NULL) {
        symtable_pop_avl(stack);
        free(stack->top);
    }
}
