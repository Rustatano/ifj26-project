#include "parse_control.h"
#include "parse_block.h"
#include "parse_expression.h"
#include <stdlib.h>
#include <string.h>

ast_node_t *parse_if(parser_context_t *context) {
    int line = context->current_token.line;
    if (!consume_token(context, TOKEN_KW_IF, "Expected 'if' keyword")) {
        return NULL;
    }

    ast_node_t *condition_node = parse_expression(context);
    if (condition_node == NULL) {
        return NULL;
    }

    if (!consume_token(context, TOKEN_KW_THEN, "Expected 'then' after if condition")) {
        ast_dispose(&condition_node);
        return NULL;
    }

    ast_node_t *if_block_node = parse_block(context);
    if (if_block_node == NULL) {
        ast_dispose(&condition_node);
        return NULL;
    }

    ast_node_t *else_block_node = NULL;
    skip_eols(context);

    if (!consume_token(context, TOKEN_KW_ELSE, "Expected 'else' after if block")) {
        ast_dispose(&condition_node);
        ast_dispose(&if_block_node);
        return NULL;
    }

    else_block_node = parse_block(context);
    if (else_block_node == NULL) {
        ast_dispose(&condition_node);
        ast_dispose(&if_block_node);
        return NULL;
    };
    ast_node_t *if_node = ast_if_create(condition_node, if_block_node, else_block_node, line);
    if (if_node == NULL) {
        ast_dispose(&condition_node);
        ast_dispose(&if_block_node);
        ast_dispose(&else_block_node);
        return parser_fail_internal(context, line);
    }
    return if_node;
}

ast_node_t *parse_while(parser_context_t *context) {
    int line = context->current_token.line;
    if (!consume_token(context, TOKEN_KW_WHILE, "Expected 'while' keyword")) {
        return NULL;
    }

    ast_node_t *condition_node = parse_expression(context);
    if (condition_node == NULL) {
        return NULL;
    }

    if (!consume_token(context, TOKEN_KW_DO, "Expected 'do' after while condition")) {
        ast_dispose(&condition_node);
        return NULL;
    }

    ast_node_t *while_block_node = parse_block(context);
    if (while_block_node == NULL) {
        ast_dispose(&condition_node);
        return NULL;
    }

    ast_node_t *while_node = ast_while_create(condition_node, while_block_node, line);
    if (while_node == NULL) {
        ast_dispose(&condition_node);
        ast_dispose(&while_block_node);
        return parser_fail_internal(context, line);
    }
    return while_node;
}

ast_node_t *parse_import(parser_context_t *context) {
    int line = context->current_token.line;
    if (!consume_token(context, TOKEN_KW_IMPORT, "Expected 'import' keyword")) {
        return NULL;
    }

    if (!expect_token(context, TOKEN_ID, "Expected module name identifier after import")) {
        return NULL;
    }
    const char *magic_ifj_constant = "ifj26";
    if (strcmp(context->current_token.attr.str_val, magic_ifj_constant) != 0) {
        free(context->current_token.attr.str_val);
        return parser_fail_syntax(context, line, "Expected 'ifj26' after import");
    }

    free(context->current_token.attr.str_val);
    load_token(context);
    if (!consume_token(context, TOKEN_DOT, "Expected '.' after 'ifj26'")) {
        return NULL;
    }
    if (!consume_token(context, TOKEN_MUL, "Expected '*' after 'ifj26.'")) {
        return NULL;
    }
    if (!check_token(context, TOKEN_EOL) && !check_token(context, TOKEN_EOF)) {
        return parser_fail_syntax(context, line, "Expected end of line after import prolog");
    }
    ast_node_t *import_node = ast_import_create("ifj26.*", line);
    if (import_node == NULL) {
        return parser_fail_internal(context, context->current_token.line);
    }
    return import_node;
}
