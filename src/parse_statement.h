/*
-------------------------------------------------------
Name of project: Translator IFJ26
Authors: Ondruška
Creation date: 14.11.2026
File: parse_statement.h

Description: Declarations for statement parsing
-------------------------------------------------------
*/

#ifndef PARSE_STATEMENT_H
#define PARSE_STATEMENT_H

#include "parser_common.h"

// Function for dispatching and parsing a single statement inside a block
ast_node_t *parse_statement(parser_context_t *context);

#endif
