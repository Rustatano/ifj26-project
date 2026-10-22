/*
-------------------------------------------------------
Name of project: Translator IFJ26
Authors: Zobal, Tajovský, Ondruška, Svoboda
Creation date: 14.10.2026
File: symtable.h

Description: Header file for symbol table as a stack of height-balanced AVL trees
-------------------------------------------------------
*/

#ifndef symtable_H
#define symtable_H

#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

// Prefixes used in keys for variable nad function
#define SYM_PREFIX_VAR "VAR_"
#define SYM_PREFIX_FUN "FUN_"

typedef enum { SYM_TYPE_VAR, SYM_TYPE_FUNC } symtable_type_t;

typedef enum {
    SYM_TYPE_ID,
    SYM_TYPE_INT,
    SYM_TYPE_DOUBLE,
    SYM_TYPE_STRING,
    SYM_TYPE_UNIT,
} symtable_datatype_t;

// Linked list representing parameters of functions
typedef struct symtable_fun_param {
    char *name;
    symtable_datatype_t type;
    struct symtable_fun_param *next;
} symtable_fun_param_t;

// Struct representing one data object in AVL
typedef struct {
    symtable_type_t type;

    // Representing variable and constant as var or function as fun
    union {
        struct {
            symtable_datatype_t type;
            bool is_mutable;
            bool is_initialized;
            int unique_id; // ID is used by code generator
        } var;

        struct {
            symtable_datatype_t return_type;
            int param_count; // Saved because counting by looping through linked list can be slow
            symtable_fun_param_t *params;
        } fun;
    };
} symtable_data_t;

// Node of AVL
typedef struct symtable_node {
    char *key; // Search key will be Prefix (VAR_ or FUN_) and identifier passed by token
    symtable_data_t data;

    int height;                  // Height of the AVL for rotation purpose
    struct symtable_node *left;  // Keys smaller by strcmp
    struct symtable_node *right; // Keys bigger by strcmp
} symtable_node_t;

// One node of Stack of AVL representing one layer of scope of input code
typedef struct symtable_stack_item {
    symtable_node_t *tree_root;
    struct symtable_stack_item *next; // Pointer to higher scope AVL
} symtable_stack_item_t;

// Stack of AVL trees implemented as Linked list
typedef struct symtable_stack {
    symtable_stack_item_t *top;    // Representing the deepest scope
    symtable_stack_item_t *global; // Representing the highest scope
} symtable_stack_t;

// Allocate memory for Symbol table and created first AVL for global scope
bool symtable_init(symtable_stack_t *stack);

// Create new empty AVL and push it on top of symbol table stack
bool symtable_create_avl(symtable_stack_t *stack);

// Delete the AVL that is currently on top of symbol table stack
void symtable_pop_avl(symtable_stack_t *stack);

// Insert item to highest AVL
bool symtable_insert_item(symtable_stack_t *stack, symtable_type_t type, char *id,
                          symtable_data_t data);

// Search item through every scope from top to bottom/global
symtable_node_t *symtable_search(symtable_stack_t *stack, symtable_type_t type, char *id);

// Search item in the current top AVL only
symtable_node_t *symtable_stack_search_current(symtable_stack_item_t *stack_top,
                                               symtable_type_t type, char *id);

// Delete whole symbol table and free all the memory
void symtable_stack_delete(symtable_stack_t *stack);

#endif
