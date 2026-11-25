/*
-------------------------------------------------------
Name of project: Translator IFJ26
Authors: xondrus00 (Sebastian Ondruska)
Creation date: 14.11.2026
File: parse_expression.h

Description: Declarations for expression parsing
-------------------------------------------------------
*/

#ifndef PARSE_EXPRESSION_H
#define PARSE_EXPRESSION_H

#include "parser_common.h"
// Function for parsing expressions using precedence syntax analysis
ast_node_t *parse_expression(parser_context_t *context);
#endif
