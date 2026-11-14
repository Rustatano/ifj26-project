/*
-------------------------------------------------------
Name of project: Translator IFJ26
Authors: Ondruška
Creation date: 14.11.2026
File: parse_expression_stack.h

Description: Declarations for the expression precedence stack
-------------------------------------------------------
*/

#ifndef PARSE_EXPRESSION_STACK_H
#define PARSE_EXPRESSION_STACK_H

#include "parse_expression_common.h"

// Function for pushing a new item (terminal, non-terminal, or '<') onto the precedence stack
bool parser_prec_stack_push(parser_prec_stack_t *stack, parser_prec_stack_item_type_t item_type,
                            parser_prec_table_index_t prec_idx, token_type_t token_type, int line,
                            ast_node_t *node);

// Function for finding the topmost terminal on the precedence stack (skips non-terminals and '<')
parser_prec_stack_item_t *parser_prec_stack_top_terminal(parser_prec_stack_t *stack);
// Function for inserting the handle mark '<' immediately above the topmost terminal on the stack
bool parser_prec_stack_insert_handle_mark(parser_prec_stack_t *stack);
// Function for popping and freeing the top item from the precedence stack
void parser_prec_stack_pop(parser_prec_stack_t *stack);
// Function for freeing all items on the precedence stack and disposing of their AST nodes
void parser_prec_stack_dispose(parser_prec_stack_t *stack);
// Function for checking if the stack is completely reduced to a single non-terminal above '$'
bool parser_prec_stack_is_reduced(parser_prec_stack_t *stack);
// Function for checking if '+' or '-' after the current stack top is a binary operator
bool parser_prec_stack_is_binary_after_top(parser_prec_stack_t *stack);

#endif
