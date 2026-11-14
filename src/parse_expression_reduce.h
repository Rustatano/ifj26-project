#ifndef PARSE_EXPRESSION_REDUCE_H
#define PARSE_EXPRESSION_REDUCE_H

#include "parse_expression_stack.h"

bool parser_prec_table_reduce(parser_context_t *context, parser_prec_stack_t *stack);
#endif
