/*
-------------------------------------------------------
Name of project: Translator IFJ26
Authors: Zobal, Tajovský, Ondruška, Svoboda
Creation date: 10.10.2026
File: parser.h

Description: Header file for the parser
-------------------------------------------------------
*/

#ifndef PARSER_H
#define PARSER_H

#include "ast.h"
#include "symbol_table.h"
#include <stdbool.h>
#include <stdio.h>

typedef enum {
    PARSE_SUCCESS,
    PARSE_ERR_LEXICAL,
    PARSE_ERR_SYNTAX,
    PARSE_ERR_INTERNAL
} parse_error_code;

typedef struct {
    bool success;
    parse_error_code error_code;
    int error_line;
    const char *error_msg;
} parse_result_t;

// Start the parsing process. Will call the scanner get_token function and create a AST
parse_result_t parse(ast_node_t **ast_root);

#endif
