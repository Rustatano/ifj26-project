/*
-------------------------------------------------------
Name of project: Translator IFJ26
Authors: Zobal, Tajovský, Ondruška, Svoboda
Creation date: 19.10.2026
File: symbol_table.h

Description: Implementation file for symbol table as a stack of height-balanced AVL trees
-------------------------------------------------------
*/

#include <string.h>

#include "symbol_table.h"

bool symbol_table_init(symbol_table_stack_t *stack) {
    stack->top = NULL;
    return true;
}

bool symbol_table_create_avl(symbol_table_stack_t *stack) {
    if (stack == NULL) {
        return false;
    }

    // Allocation of new item of symtable
    symbol_table_stack_item_t *new_item =
        (symbol_table_stack_item_t *)malloc(sizeof(symbol_table_stack_item_t));
    if (new_item == NULL) {
        return false;
    }

    new_item->tree_root = NULL;

    new_item->next = stack->top;
    stack->top = new_item;

    return true;
}

int max_height(symbol_table_node_t *left, symbol_table_node_t *right) {
    int l = 0;
    int r = 0;

    if (left != NULL) {
        l = left->height;
    }
    if (right != NULL) {
        r = right->height;
    }

    if (l < r) {
        return r;
    }

    return l;
}

symbol_table_node_t *insert_node(symbol_table_node_t *root, symbol_table_type_t type, char *id,
                                 symbol_table_data_t data, bool *success) {
    if (root == NULL) {
        // Allocation of memory for the node
        symbol_table_node_t *node = (symbol_table_node_t *)malloc(sizeof(symbol_table_node_t));
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

    // Update height
    root->height = max_height(root->left, root->right) + 1;

    return root; // return root without change
}

bool symbol_table_insert_item(symbol_table_stack_t *stack, symbol_table_type_t type, char *id,
                              symbol_table_data_t data) {
    bool success = true;

    stack->top->tree_root = insert_node(stack->top->tree_root, type, id, data, &success);

    return success;
}

symbol_table_node_t *sym_stack_search_current(symbol_table_stack_item_t *stack_top,
                                              symbol_table_type_t type, char *id) {
    if (stack_top == NULL) {
        return NULL;
    }

    symbol_table_node_t *root = stack_top->tree_root;
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

symbol_table_node_t *symbol_table_search(symbol_table_stack_t *stack, symbol_table_type_t type,
                                         char *id) {
    if (stack == NULL) {
        return NULL;
    }

    symbol_table_stack_item_t *top = stack->top;
    if (top == NULL) {
        return NULL;
    }

    symbol_table_node_t *node = NULL;
    while (top != NULL) {
        node = sym_stack_search_current(top, type, id);
        if (node != NULL) {
            return node;
        }

        top = top->next;
    }

    return NULL;
}

// Recursively delete node and its children
void delete_node(symbol_table_node_t *node) {
    if (node == NULL) {
        return;
    }

    delete_node(node->left);
    delete_node(node->right);

    free(node->key);

    // Delete function parameters and its names
    if (node->data.type == SYM_TYPE_FUNC) {
        symbol_table_fun_param_t *param = node->data.fun.params;
        symbol_table_fun_param_t *next_param;
        while (param != NULL) {
            next_param = param->next;
            free(param->name);
            free(param);
            param = next_param;
        }
    }

    free(node);
}

void symbol_table_pop_avl(symbol_table_stack_item_t *stack_top) {
    if (stack_top == NULL) {
        return;
    }

    delete_node(stack_top->tree_root);
}

void sym_stack_delete(symbol_table_stack_t *stack) {
    if (stack == NULL) {
        return;
    }

    symbol_table_stack_item_t *top = stack->top;
    symbol_table_stack_item_t *next_top;

    // Delete each AVL tree
    while (top != NULL) {
        symbol_table_pop_avl(top);
        next_top = top->next;
        free(top);
        top = next_top;
    }

    stack->top = NULL;
}
