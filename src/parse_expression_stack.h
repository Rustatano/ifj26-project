#ifndef PARSE_EXPRESSION_STACK_H
#define PARSE_EXPRESSION_STACK_H

#include "parse_expression_common.h"

bool parser_prec_stack_push(parser_prec_stack_t *stack, parser_prec_stack_item_type_t item_type,
                            parser_prec_table_index_t prec_idx, token_type_t token_type, int line,
                            ast_node_t *node);

parser_prec_stack_item_t *parser_prec_stack_top_terminal(parser_prec_stack_t *stack);
bool parser_prec_stack_insert_handle_mark(parser_prec_stack_t *stack);
void parser_prec_stack_pop(parser_prec_stack_t *stack);
void parser_prec_stack_dispose(parser_prec_stack_t *stack);
bool parser_prec_stack_is_reduced(parser_prec_stack_t *stack);
bool prec_stack_is_binary_after_top(parser_prec_stack_t *stack);

#endif
