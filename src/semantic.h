/*
-------------------------------------------------------
Name of project: Translator IFJ26
Authors: Zobal, Tajovský, Ondruška, Svoboda
Creation date: 18.10.2026
File: semantic.h

Description: Header file for semantic analyzer

-------------------------------------------------------
*/

#include <stdbool.h>

#include "ast.h"
#include "symbol_table.h"

bool semantic_init(symtable_stack_t *symtable_stack);

int count_fun_params(symtable_fun_param_t *param);

symtable_datatype_t check_expres_var(symtable_stack_t *symtable_stack, ast_node_t *node);

symtable_datatype_t check_expres_fun_call(symtable_stack_t *symtable_stack, ast_node_t *node);

ast_node_t *create_conversion_node(ast_node_t *from);

symtable_datatype_t implicit_conversion_int_to_double(ast_node_t **node_ptr);

symtable_datatype_t check_expres_operator(symtable_stack_t *symtable_stack, ast_node_t *node);

symtable_datatype_t check_expr(symtable_stack_t *symtable_stack, ast_node_t *node);

symtable_datatype_t check_var_declaration(symtable_stack_t *symtable_stack, ast_node_t *node);

symtable_datatype_t check_value_assignment(symtable_stack_t *symtable_stack, ast_node_t *node);

symtable_datatype_t check_statement_if(symtable_stack_t *symtable_stack, ast_node_t *node,
                                       symtable_node_t *fun_context);

symtable_datatype_t check_statement_while(symtable_stack_t *symtable_stack, ast_node_t *node,
                                          symtable_node_t *fun_context);

symtable_datatype_t check_statement_return(symtable_stack_t *symtable_stack, ast_node_t *node,
                                           symtable_node_t *fun_context);

symtable_datatype_t check_block(symtable_stack_t *symtable_stack, ast_node_t *node,
                                symtable_node_t *fun_context);

bool first_ast_passage(ast_node_t *ast, symtable_stack_t *symtable_stack);

bool second_ast_passage(ast_node_t *ast, symtable_stack_t *symtable_stack);

bool semantic_analysis(ast_node_t *ast, symtable_stack_t *symtable_stack);
