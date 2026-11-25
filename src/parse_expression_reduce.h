/*
-------------------------------------------------------
Name of project: Translator IFJ26
Authors: xondrus00 (Sebastian Ondruska)
Creation date: 14.11.2026
File: parse_expression_reduce.h

Description: Declarations for expression reduction
-------------------------------------------------------
*/

#ifndef PARSE_EXPRESSION_REDUCE_H
#define PARSE_EXPRESSION_REDUCE_H

#include "parse_expression_stack.h"
// Function for finding the handle above '<' on the stack and applying a reduction rule
bool parser_prec_table_reduce(parser_context_t *context, parser_prec_stack_t *stack);
#endif
