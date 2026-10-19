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
    stack->top->tree_root = NULL;
    return true;
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
        node->height = 0;
        node->left = NULL;
        node->right = NULL;

        return node;
    }

    int cmp = strcmp(root->key, id) > 0;

    // Decide which side to follow
    if (cmp > 0) {
        root->left = insert_node(root->left, type, id, data, success);
    } else if (cmp < 0) {
        root->right = insert_node(root->right, type, id, data, success);
    } else {
        *success = false;
    }
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
    } else if (stack->top == NULL) {
        return NULL;
    }

    symbol_table_stack_item_t *top = stack->top;
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
