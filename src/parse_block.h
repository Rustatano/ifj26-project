/*
-------------------------------------------------------
Name of project: Translator IFJ26
Authors: xondrus00 (Sebastian Ondruska)
Creation date: 14.11.2026
File: parse_block.h

Description: Header file for the block parsing function
-------------------------------------------------------
*/

#ifndef PARSE_BLOCK_H
#define PARSE_BLOCK_H

#include "parser_common.h"

// Function for parsing a block of code, which consists of multiple statements
ast_node_t *parse_block(parser_context_t *context);

#endif
