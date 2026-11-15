/*
-------------------------------------------------------
Name of project: Translator IFJ26
Authors: xsvoboj01 (Jakub Svoboda)
Creation date: 18.10.2026
File: semantic.h

Description: Header file for semantic analyzer

-------------------------------------------------------
*/

#ifndef SEMANTIC_H
#define SEMANTIC_H

#include <stdbool.h>

#include "ast.h"
#include "symtable.h"

typedef struct {
    bool success;
    int error_code;
    int error_line;
    const char *error_msg;
} semantic_result_t;

typedef struct {
    int unique_var_id;
    semantic_result_t res;
} semantic_context_t;

typedef struct {
    const char *name;
    int param_count;
    symtable_datatype_t param_types[3];
    symtable_datatype_t return_type;
} builtin_fun_t;

// Report semantic error
void set_semantic_error(semantic_context_t *ctx, int code, const char *msg, int line);

// Inserts supported builtin function declarations to global symtable
bool include_builtin_funs(semantic_context_t *ctx, symtable_stack_t *symtable_stack);

// Initialize global symtable
bool semantic_init(semantic_context_t *ctx, symtable_stack_t *symtable_stack);

// Count function parameters recursively
int count_fun_params(symtable_fun_param_t *param);

// Check if variable is defined
symtable_datatype_t check_expres_var(semantic_context_t *ctx, symtable_stack_t *symtable_stack,
                                     ast_node_t *node);

// Check if function is defined and calling is correct
symtable_datatype_t check_expres_fun_call(semantic_context_t *ctx, symtable_stack_t *symtable_stack,
                                          ast_node_t *node);

// Allocate and initialize AST_NODE_CONVERT
ast_node_t *create_conversion_node(semantic_context_t *ctx, ast_node_t *from);

// Do implicit conversion if allowed
symtable_datatype_t implicit_conversion_int_to_double(semantic_context_t *ctx,
                                                      ast_node_t **node_ptr);

// Check if left and right side of operator has the same type
symtable_datatype_t check_expres_operator(semantic_context_t *ctx, symtable_stack_t *symtable_stack,
                                          ast_node_t *node);

// Determine type of expression recursively
symtable_datatype_t check_expr(semantic_context_t *ctx, symtable_stack_t *symtable_stack,
                               ast_node_t *node);

// Check if variable is not declared and initialize it
symtable_datatype_t check_var_declaration(semantic_context_t *ctx, symtable_stack_t *symtable_stack,
                                          ast_node_t *node, symtable_node_t *fun_ctx);

// Check if variable is declared and assign to it
symtable_datatype_t check_value_assignment(semantic_context_t *ctx,
                                           symtable_stack_t *symtable_stack, ast_node_t *node);

// Check if statement blocks and condition
symtable_datatype_t check_statement_if(semantic_context_t *ctx, symtable_stack_t *symtable_stack,
                                       ast_node_t *node, symtable_node_t *fun_context);

// Check while statement blocks and condition
symtable_datatype_t check_statement_while(semantic_context_t *ctx, symtable_stack_t *symtable_stack,
                                          ast_node_t *node, symtable_node_t *fun_context);

// Check correctness of return type
symtable_datatype_t check_statement_return(semantic_context_t *ctx,
                                           symtable_stack_t *symtable_stack, ast_node_t *node,
                                           symtable_node_t *fun_context);

// Check statements in code block sequentially
symtable_datatype_t check_block(semantic_context_t *ctx, symtable_stack_t *symtable_stack,
                                ast_node_t *node, symtable_node_t *fun_context);

// First passage of AST saves global definitions of functions
bool first_ast_passage(semantic_context_t *ctx, symtable_stack_t *symtable_stack, ast_node_t *ast);

// Second pasasge of AST checks every block and statement
bool second_ast_passage(semantic_context_t *ctx, symtable_stack_t *symtable_stack, ast_node_t *ast);

// Wrapper function to call in main()
semantic_result_t semantic_analysis(ast_node_t *ast, symtable_stack_t *symtable_stack);

#endif // SEMANTIC_H
