/*
-------------------------------------------------------
Name of project: Translator IFJ26
Authors: Ondruška
Creation date: 14.11.2026
File: parse_statement.c

Description: Statement parsing implementation
-------------------------------------------------------
*/

#include "parse_statement.h"
#include "parse_control.h"
#include "parse_declaration.h"
#include "parse_expression.h"
#include "parse_function.h"

// Function for dispatching and parsing a single statement inside a block
// {var_declaration}
// {assignment}
// {if_statement}
// {while_statement}
// {return_statement}
// {expression}
ast_node_t *parse_statement(parser_context_t *context) {
    switch (context->current_token.type) {
    case TOKEN_KW_VAR:
    case TOKEN_KW_VAL:
        return parse_var_declaration(context);
    case TOKEN_KW_IF:
        return parse_if(context);
    case TOKEN_KW_WHILE:
        return parse_while(context);
    case TOKEN_KW_RETURN:
        return parse_return(context);
    case TOKEN_ID: {
        token_t token_id = context->current_token;
        load_token(context);

        if (check_token(context, TOKEN_ASSIGN)) {
            unread_token(context, token_id);
            return parse_assignment(context);
        }

        unread_token(context, token_id);
        return parse_expression(context);
    }
    case TOKEN_INT:
    case TOKEN_DOUBLE:
    case TOKEN_STRING:
    case TOKEN_PLUS:
    case TOKEN_MINUS:
    case TOKEN_LPAREN:
        return parse_expression(context);
    default:
        return parser_fail_syntax(context, context->current_token.line,
                                  "Unexpected token at start of statement");
    }
}
