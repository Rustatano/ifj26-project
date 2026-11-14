/*
-------------------------------------------------------
Name of project: Translator IFJ26
Authors: Ondruška
Creation date: 14.11.2026
File: parse_control.h

Description: Declarations for control statement parsing
-------------------------------------------------------
*/

#ifndef PARSE_CONTROL_H
#define PARSE_CONTROL_H

#include "parser_common.h"

// Function for parsing an if-else conditional statement
ast_node_t *parse_if(parser_context_t *context);
// Function for parsing a while loop statement
ast_node_t *parse_while(parser_context_t *context);
// Function for parsing the mandatory prolog statement at the start of the program
ast_node_t *parse_import(parser_context_t *context);

#endif
