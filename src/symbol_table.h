/*
-------------------------------------------------------
Name of project: Translator IFJ26
Authors: Zobal, Tajovský, Ondruška, Svoboda
Creation date: 14.10.2026
File: symbol_table.h

Description: Header file for symbol table as a stack of height-balanced AVL trees
-------------------------------------------------------
*/

#ifndef SYMBOL_TABLE_H
#define SYMBOL_TABLE_H

#include <stdbool.h>
#include <stddef.h>


// Prefixes used in keys for variable nad function
#define SYM_PREFIX_VAR "VAR_"
#define SYM_PREFIX_FUN "FUN_"

typedef enum {
    SYM_TYPE_VAR,
    SYM_TYPE_FUNC
} symbol_table_type_t;


typedef enum {
	SYM_TYPE_ID,
    SYM_TYPE_INT,
    SYM_TYPE_DOUBLE,
    SYM_TYPE_STRING,
    SYM_TYPE_UNIT,
} symbol_table_datatype_t;



// Linked list representing parametrs of functions
typedef struct symbol_table_fun_param {
    char *name;
    symbol_table_datatype_t type;
    struct symbol_table_fun_param *next;
} symbol_table_fun_param_t;

// Struct representing one data object in AVL
typedef struct {
    symbol_table_type_t type;

    // Representing variable and constant as var or function as fun
    union {
        struct {
            symbol_table_datatype_t type;
            bool is_mutable;
            bool is_initialized;
            int unique_id;  // ID is used be code generator
        } var;

        struct {
            symbol_table_datatype_t return_type;
            int param_count;  // Saved because counting by looping thought linked list can be slow
            symbol_table_fun_param_t *params;
        } fun;
    };
} symbol_table_data_t;

// Node of AVL
typedef struct symbol_table_node {
    char *key;   // Search key will be Prefix (VAR_ or FUN_) and identifier passed be token
    symbol_table_data_t data;

    int height;  // Height of the AVL for rotation purpose
    struct symbol_table_node *left;     // Keys smaller by strcmp
    struct symbol_table_node *right;    // Keys bigger by strcmp
} symbol_table_node_t;


// One node of Stack of AVL representing one layer of scope of input code
typedef struct symbol_table_stack_item {
    symbol_table_node_t *tree_root;
    struct symbol_table_stack_item *next; // Pointer to higher scope AVL
} symbol_table_stack_item_t;

// Stack of AVL trees implemented as Linked list
typedef struct symbol_table_stack {
    symbol_table_stack_item_t *top;     // Representing the deepeast scope
    symbol_table_stack_item_t *global;  // Bottom of stack representing global (heighes) scope
} symbol_table_stack_t;


// Allocate memory for Symbol table and created first AVL for global scope
bool symbol_table_init(symbol_table_stack_t **stack);

// Create new empty AVL and push it on top of symbol table stack
bool symbol_table_create_avl(symbol_table_stack_t *stack);

// Delete the AVL that is currently on top of symbol table stack
void symbol_table_pop_avl(symbol_table_stack_t *stack);

// Insert item to heighest AVL
bool symbol_table_insert_item(symbol_table_stack_t *stack, symbol_table_type_t type, char* identifier, symbol_table_data_t data);

// Search item throught every scope from top to bottom/global
symbol_table_node_t *symbol_table_search(symbol_table_stack_t *stack, symbol_table_type_t type,  char *identifier);

// Search item in the current top AVL only
symbol_table_node_t *sym_stack_search_current(symbol_table_stack_t *stack, symbol_table_type_t type,  char *identifier);

// Delete whole symbol table and free all the memory
void sym_stack_delete(symbol_table_stack_t **stack);


#endif
