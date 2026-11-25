/*
-------------------------------------------------------
Name of project: Translator IFJ26
Authors: xondrus00 (Sebastian Ondruska)
Creation date: 14.11.2026
File: parser_common.h

Description: Common parser declarations and parser context
-------------------------------------------------------
*/

#ifndef PARSER_COMMON_H
#define PARSER_COMMON_H

#include "parser.h"

typedef struct parser_context {
    token_t current_token;
    int error_line;
    const char *error_message;
    parse_error_code error_code;
    scanner_conf_t *scanner_conf;
    token_t buffered_token;
    bool has_buffered_token;
} parser_context_t;

// Function for loading the next token into context (from lookahead buffer or scanner)
void load_token(parser_context_t *context);
// Function for pushing the current token into a single-token lookahead buffer
// and restoring the given token as the current token
void unread_token(parser_context_t *context, token_t token);
// Function for parsing a comma-separated list of arguments in a function call
// ()
// ({expression}, {expression}, ...)
// ({expression}, {EOL})
bool parse_argument_list(parser_context_t *context, ast_node_t **params, int *params_count);
// Function for checking if the current token matches the expected token type without consuming it
bool check_token(parser_context_t *context, token_type_t token_type);
// Function for verifying the current token type; records a syntax error if it does not match
bool expect_token(parser_context_t *context, token_type_t token_type, const char *error_message);
// Function for checking if a token can terminate a statement (EOL, ';', DEDENT, or EOF)
bool is_statement_terminator(token_type_t type);
// Function for freeing a linked list of function parameters and their dynamically allocated names
void dispose_parameters(symtable_fun_param_t **parameters);
// Function for recording an internal compiler error
void *parser_fail_internal(parser_context_t *context, int error_line);
// Function for recording a syntax error in the parser context without overwriting internal errors
void *parser_fail_syntax(parser_context_t *context, int error_line, const char *error_message);
// Function for consuming the current token if it matches expected, or recording a syntax error
bool consume_token(parser_context_t *context, token_type_t expected, const char *error_message);
// Function for consuming all consecutive end-of-line (EOL) tokens
void skip_eols(parser_context_t *context);

#endif
