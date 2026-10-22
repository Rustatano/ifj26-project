/*
-------------------------------------------------------
Name of project: Translator IFJ26
Authors: Zobal, Tajovský, Ondruška, Svoboda
Creation date: 10.10.2026
File: parser.c

Description: Parser implementation.

-------------------------------------------------------
*/
#include "parser.h"
#include "ast.h"
#include "scanner.h"
#include <stdbool.h>

typedef struct parser_context {
    token_t current_token;
    int error_line;
    const char *error_message;
    scanner_conf_t *scanner_conf;
} parser_context_t;

parse_result_t parse(ast_node_t **ast_root, scanner_conf_t *scanner_conf) {
    // Create a context
    //
    // Start parsing loop
    //
    // 			Call the correct parse functions
    //          Connect the root to main tree
    // return the tree
}

void load_token(parser_context_t *context) {
    token_t loaded_token = get_token(context->scanner_conf);
    context->current_token = loaded_token;
}

bool verify_token_type(parser_context_t *context, token_type_t token_type) {
    if (context->current_token.type == token_type) {
        return true;
    }
    context->error_line = context->current_token.line;
    context->error_message = "Generic syntax error message. Grrrr";
    return false;
}

ast_node_t *parse_import(parser_context_t *context) {

    if (!verify_token_type(context, TOKEN_KEYWORD_IMPORT)) {
        return NULL;
    }

    load_token(context);
    if (!verify_token_type(context, TOKEN_ID)) {
        return NULL;
    }
    char *module_name = context->current_token.attr.str_val;

    load_token(context);
    if (!verify_token_type(context, TOKEN_EOL)) {
        return NULL;
    }
    ast_node_t *new_node = ast_node_create(AST_NODE_STATEMENT_IMPORT, context->current_token.line);
    new_node->import_statement.module_name = module_name;

    return new_node;
}
