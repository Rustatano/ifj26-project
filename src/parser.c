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

// Forward declarations
ast_node_t *parse_term(parser_context_t *context);
ast_node_t *parse_unary(parser_context_t *context);
ast_node_t *parse_factor(parser_context_t *context);

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

ast_expres_operator_t remap_operator(token_type_t operator) {
    switch (operator) {
    case TOKEN_PLUS:
        return AST_EXPRES_OPERATOR_ADD;
    case TOKEN_MINUS:
        return AST_EXPRES_OPERATOR_SUB;
    case TOKEN_MUL:
        return AST_EXPRES_OPERATOR_MUL;
    case TOKEN_DIV:
        return AST_EXPRES_OPERATOR_DIV;
    default:
        return AST_EXPRES_OPERATOR_ADD;
    }
}

// Returns NULL on syntax error
ast_node_t *parse_expresion(parser_context_t *context) {
    ast_node_t *left = parse_term(context);
    if (left == NULL) {
        return NULL;
    }

    while (context->current_token.type == TOKEN_PLUS ||
           context->current_token.type == TOKEN_MINUS) {
        token_type_t operator= context->current_token.type;
        int line = context->current_token.line;
        load_token(context);
        ast_node_t *right = parse_term(context);
        if (right == NULL) {
            return NULL;
        }
        left = ast_expres_operation_create(remap_operator(operator), left, right, line);
    }

    return left;
}

ast_node_t *parse_term(parser_context_t *context) {
    ast_node_t *left = parse_unary(context);
    if (left == NULL) {
        return NULL;
    }

    while (context->current_token.type == TOKEN_MUL || context->current_token.type == TOKEN_DIV) {
        token_type_t operator= context->current_token.type;
        int line = context->current_token.line;
        load_token(context);
        ast_node_t *right = parse_unary(context);
        if (right == NULL) {
            return NULL;
        }
        left = ast_expres_operation_create(remap_operator(operator), left, right, line);
    }

    return left;
}

ast_node_t *parse_unary(parser_context_t *context) {
    if (context->current_token.type == TOKEN_PLUS) {
        load_token(context);
        return parse_unary(context);
    }

    if (context->current_token.type == TOKEN_MINUS) {
        int line = context->current_token.line;
        load_token(context);

        ast_node_t *operand = parse_unary(context);
        if (operand == NULL) {
            return NULL;
        }

        return ast_expres_operation_create(AST_EXPRES_OPERATOR_SUB, ast_const_int_create(0, line),
                                           operand, line);
    }

    return parse_factor(context);
}

ast_node_t *parse_factor(parser_context_t *context) {
    token_t token = context->current_token;

    switch (token.type) {
    case TOKEN_INT: {
        ast_node_t *node = ast_const_int_create(token.attr.int_val, token.line);
        load_token(context);
        return node;
    }

    case TOKEN_DOUBLE: {
        ast_node_t *node = ast_const_double_create(token.attr.double_val, token.line);
        load_token(context);
        return node;
    }

    case TOKEN_STRING: {
        ast_node_t *node = ast_const_string_create(token.attr.str_val, token.line);
        load_token(context);
        return node;
    }

    case TOKEN_ID: {
        char *name = token.attr.str_val;
        ast_node_t *node = ast_expres_var_create(name, 0, SYM_DATATYPE_ID, token.line);
        load_token(context);
        return node;
    }

    case TOKEN_LPAREN: {
        load_token(context);

        ast_node_t *expression = parse_expresion(context);
        if (expression == NULL) {
            return NULL;
        }

        if (!verify_token_type(context, TOKEN_RPAREN)) {
            return NULL;
        }

        load_token(context);

        return expression;
    }

    default:
        context->error_line = token.line;
        context->error_message = "Expected expression factor";
        return NULL;
    }
}
