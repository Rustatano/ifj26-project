/*
-------------------------------------------------------
Name of project: Translator IFJ26
Authors: Ondruška
Creation date: 14.11.2026
File: parse_declaration.h

Description: Declarations for variable parsing
-------------------------------------------------------
*/

#ifndef PARSE_DECLARATION_H
#define PARSE_DECLARATION_H

#include "parser_common.h"

// Function for parsing a mutable (var) or immutable (val) variable declaration
ast_node_t *parse_var_declaration(parser_context_t *context);
// Function for parsing a variable assignment statement
ast_node_t *parse_assignment(parser_context_t *context);
// Function for converting token type to symbol table type
symtable_datatype_t parse_data_type(parser_context_t *context);

#endif
