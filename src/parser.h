/*
-------------------------------------------------------
Name of project: Translator IFJ26
Authors: Ondruška
Creation date: 10.10.2026
File: parser.h

Description: Header file for the parser
-------------------------------------------------------
*/

#ifndef PARSER_H
#define PARSER_H

#include "ast.h"
#include "scanner.h"
#include <stdbool.h>
#include <stdio.h>

#define PARSE_SUCCESS 0
#define PARSE_ERR_LEXICAL 1
#define PARSE_ERR_SYNTAX 2
#define PARSE_ERR_INTERNAL 99

typedef struct {
    bool success;
    int error_code;
    int error_line;
    const char *error_msg;
} parse_result_t;

// Start the parsing process. Will call the scanner get_token function and create a AST
parse_result_t parse(ast_node_t **ast_root, scanner_conf_t *scanner_conf);

#endif
