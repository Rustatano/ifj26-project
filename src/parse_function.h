/*
-------------------------------------------------------
Name of project: Translator IFJ26
Authors: Ondruška
Creation date: 14.11.2026
File: parse_function.h

Description: Declarations for function parsing
-------------------------------------------------------
*/

#ifndef PARSE_FUNCTION_H
#define PARSE_FUNCTION_H

#include "parser_common.h"

// Function for parsing a single-line or multi-line function definition
ast_node_t *parse_fun_definition(parser_context_t *context, bool is_main);
// Function for parsing a return statement with an optional expression
ast_node_t *parse_return(parser_context_t *context);
#endif
