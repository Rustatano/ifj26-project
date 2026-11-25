/*
-------------------------------------------------------
Name of project: Translator IFJ26
Authors: xondrus00 (Sebastian Ondruska)
Creation date: 14.11.2026
File: parse_block.c

Description: Implementation of the block parsing function
-------------------------------------------------------
*/

#include "parse_block.h"
#include "parse_statement.h"

// Function for parsing an indented block of statements separated by EOL or ';'
// {EOLS}
// {INDENT}
//   {statement} [; {statement}] {EOL}
//   {statement}
// {DEDENT | EOF}
ast_node_t *parse_block(parser_context_t *context) {
    int line = context->current_token.line;
    skip_eols(context);

    if (!consume_token(context, TOKEN_INDENT, "Expected indentation for block")) {
        return NULL;
    }
    ast_node_t *block_node = ast_block_create(line);
    if (block_node == NULL) {
        return parser_fail_internal(context, line);
    }

    while (!check_token(context, TOKEN_DEDENT) && !check_token(context, TOKEN_EOF)) {
        skip_eols(context);
        if (check_token(context, TOKEN_DEDENT) || check_token(context, TOKEN_EOF)) {
            break;
        }

        ast_node_t *statement_node = parse_statement(context);
        if (statement_node == NULL) {
            ast_dispose(&block_node);
            return NULL;
        }
        ast_block_append(block_node, statement_node);

        bool is_compound_statement = statement_node->type == AST_NODE_STATEMENT_IF ||
                                     statement_node->type == AST_NODE_STATEMENT_WHILE;
        if (!is_compound_statement) {
            if (check_token(context, TOKEN_EOL) || check_token(context, TOKEN_SEMICOLON)) {
                load_token(context);
            } else if (!is_statement_terminator(context->current_token.type)) {
                parser_fail_syntax(context, context->current_token.line,
                                   "Expected end of statement");
                ast_dispose(&block_node);
                return NULL;
            }
        }
    }

    if (block_node->block.first_statement == NULL) {
        parser_fail_syntax(context, line, "Empty blocks are not allowed");
        ast_dispose(&block_node);
        return NULL;
    }

    if (check_token(context, TOKEN_DEDENT)) {
        load_token(context);
    } else if (!check_token(context, TOKEN_EOF)) {
        parser_fail_syntax(context, context->current_token.line,
                           "Expected dedent at the end of block");
        ast_dispose(&block_node);
        return NULL;
    }

    return block_node;
}
