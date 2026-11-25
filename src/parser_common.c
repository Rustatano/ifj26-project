/*
-------------------------------------------------------
Name of project: Translator IFJ26
Authors: xondrus00 (Sebastian Ondruska)
Creation date: 14.11.2026
File: parser_common.c

Description: Common parser helpers and parser context operations
-------------------------------------------------------
*/

#include "parser_common.h"
#include "parse_expression.h"
#include <stdlib.h>

// Function for loading the next token into context (from lookahead buffer or scanner)
void load_token(parser_context_t *context) {
    token_t loaded_token;
    if (context->has_buffered_token) {
        loaded_token = context->buffered_token;
        context->has_buffered_token = false;
    } else {
        loaded_token = get_token(context->scanner_conf);
    }
    context->current_token = loaded_token;
    context->error_line = loaded_token.line;
}

// Function for pushing the current token into a single-token lookahead buffer
// and restoring the given token as the current token
void unread_token(parser_context_t *context, token_t token) {
    context->buffered_token = context->current_token;
    context->has_buffered_token = true;
    context->current_token = token;
}

// Function for parsing a comma-separated list of arguments in a function call
// ()
// ({expression}, {expression}, ...)
// ({expression}, {EOL})
bool parse_argument_list(parser_context_t *context, ast_node_t **params, int *params_count) {
    *params = NULL;
    ast_node_t *last_param = NULL;
    *params_count = 0;

    skip_eols(context);
    if (check_token(context, TOKEN_RPAREN)) {
        load_token(context);
        return true;
    }

    while (true) {
        ast_node_t *current_argument = parse_expression(context);
        if (current_argument == NULL) {
            ast_dispose(params);
            return false;
        }

        if (*params == NULL) {
            *params = current_argument;
        } else {
            last_param->next = current_argument;
        }
        last_param = current_argument;
        (*params_count)++;

        if (check_token(context, TOKEN_RPAREN)) {
            load_token(context);
            return true;
        }

        if (!consume_token(context, TOKEN_COMMA,
                           "Expected ',' or ')' in function call arguments")) {
            ast_dispose(params);
            return false;
        }
        skip_eols(context);

        if (check_token(context, TOKEN_RPAREN)) {
            // Trailing comma is allowed only if a line break follows it (spec 1.2)
            if (!context->current_token.newline_before) {
                ast_dispose(params);
                parser_fail_syntax(context, context->current_token.line,
                                   "Trailing comma must be followed by a line break");
                return false;
            }
            load_token(context);
            return true;
        }
    }
}

// Function for checking if the current token matches the expected token type without consuming it
bool check_token(parser_context_t *context, token_type_t token_type) {
    return context->current_token.type == token_type;
}

// Function for verifying the current token type; records a syntax error if it does not match
bool expect_token(parser_context_t *context, token_type_t token_type, const char *error_message) {
    if (context->current_token.type == token_type) {
        return true;
    }
    parser_fail_syntax(context, context->current_token.line, error_message);
    return false;
}

// Function for checking if a token can terminate a statement (EOL, ';', DEDENT, or EOF)
bool is_statement_terminator(token_type_t type) {
    return type == TOKEN_EOL || type == TOKEN_SEMICOLON || type == TOKEN_DEDENT ||
           type == TOKEN_EOF;
}

// Function for freeing a linked list of function parameters and their dynamically allocated names
void dispose_parameters(symtable_fun_param_t **parameters) {
    while (parameters != NULL && *parameters != NULL) {
        symtable_fun_param_t *next = (*parameters)->next;
        free((*parameters)->name);
        free(*parameters);
        *parameters = next;
    }
}

// Function for consuming all consecutive end-of-line (EOL) tokens
void skip_eols(parser_context_t *context) {
    while (check_token(context, TOKEN_EOL)) {
        load_token(context);
    }
}

// Function for consuming the current token if it matches expected, or recording a syntax error
bool consume_token(parser_context_t *context, token_type_t expected, const char *error_message) {
    if (check_token(context, expected)) {
        load_token(context);
        return true;
    }
    parser_fail_syntax(context, context->current_token.line, error_message);
    return false;
}

// Function for recording a syntax error in the parser context without overwriting internal errors
void *parser_fail_syntax(parser_context_t *context, int error_line, const char *error_message) {
    if (context->error_code != PARSE_ERR_INTERNAL) {
        context->error_code = PARSE_ERR_SYNTAX;
        context->error_line = error_line;
        context->error_message = error_message;
    }
    return NULL;
}

// Function for recording an internal compiler error
void *parser_fail_internal(parser_context_t *context, int error_line) {
    context->error_code = PARSE_ERR_INTERNAL;
    context->error_line = error_line;
    context->error_message = "Memory allocation failed";
    return NULL;
}
