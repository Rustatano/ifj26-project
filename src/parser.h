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

// Forward declaration of AST node
typedef struct ast_node ast_node_t;

/**
 * Runs syntax analysis.
 *
 * @param ast_root Pointer to the AST root pointer where the created tree will be stored.
 * @return parse_result_t Status of analysis, error code, line number, and error message.
 */
parse_result_t parse(ast_node_t **ast_root);

#endif
