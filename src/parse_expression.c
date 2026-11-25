/*
-------------------------------------------------------
Name of project: Translator IFJ26
Authors: xondrus00 (Sebastian Ondruska)
Creation date: 14.11.2026
File: parse_expression.c

Description: Expression parsing implementation
-------------------------------------------------------
*/

#include "parse_expression.h"
#include "parse_expression_common.h"
#include "parse_expression_reduce.h"
#include "parse_expression_stack.h"
#include <stdlib.h>

// Function for parsing a single operand in an expression (literal, variable, or function call)
// {Int | Double | String}
// {id}
// {id}([{expression}, ...])
ast_node_t *parse_operand(parser_context_t *context) {
    token_t token = context->current_token;

    switch (token.type) {
    case TOKEN_INT:
        load_token(context);
        ast_node_t *int_node = ast_const_int_create(token.attr.int_val, token.line);
        return int_node ? int_node : parser_fail_internal(context, token.line);

    case TOKEN_DOUBLE:
        load_token(context);
        ast_node_t *double_node = ast_const_double_create(token.attr.double_val, token.line);
        return double_node ? double_node : parser_fail_internal(context, token.line);

    case TOKEN_STRING:
        load_token(context);
        ast_node_t *string_node = ast_const_string_create(token.attr.str_val, token.line);
        if (string_node == NULL) {
            free(token.attr.str_val);
            return parser_fail_internal(context, token.line);
        }
        return string_node;

    case TOKEN_ID: {
        char *name = token.attr.str_val;
        int line = token.line;
        load_token(context);

        if (check_token(context, TOKEN_LPAREN)) {
            load_token(context);
            int params_count = 0;
            ast_node_t *params = NULL;
            if (!parse_argument_list(context, &params, &params_count)) {
                free(name);
                return NULL;
            }
            ast_node_t *fun_call = ast_expression_fun_call_create(name, params, params_count, line);
            if (fun_call == NULL) {
                free(name);
                ast_dispose(&params);
                return parser_fail_internal(context, context->current_token.line);
            }
            return fun_call;
        }
        ast_node_t *variable_node = ast_expression_var_create(name, 0, SYM_DATATYPE_ID, line);
        if (variable_node == NULL) {
            free(name);
            return parser_fail_internal(context, token.line);
        }
        return variable_node;
    }

    default:
        return parser_fail_syntax(context, token.line, "Expected operand in expression");
    }
}

// Function for parsing expressions using precedence syntax analysis
// ()
// ({expression})
// {+ | -}{expression}
// {expression} {* | / | + | - | < | <= | > | >= | == | !=} {expression}
ast_node_t *parse_expression(parser_context_t *context) {
    parser_prec_stack_t stack;
    stack.top = NULL;

    if (parser_prec_stack_push(&stack, PARSER_PREC_STACK_ITEM_TERMINAL, PARSER_PREC_DOLLAR,
                               TOKEN_EOF, context->current_token.line, NULL) == false) {
        return parser_fail_internal(context, context->current_token.line);
    }

    while (true) {
        parser_prec_stack_item_t *top_terminal = parser_prec_stack_top_terminal(&stack);
        parser_prec_table_index_t current_prec_index =
            parser_prec_table_token_to_prec_index(context, &stack);

        if (current_prec_index == PARSER_PREC_DOLLAR && parser_prec_stack_is_reduced(&stack)) {
            ast_node_t *result = stack.top->node;
            stack.top->node = NULL;
            parser_prec_stack_dispose(&stack);
            return result;
        }

        char relation = parser_prec_table[top_terminal->index][current_prec_index];

        if (relation == '=') {
            if (!parser_prec_stack_push(&stack, PARSER_PREC_STACK_ITEM_TERMINAL, current_prec_index,
                                        context->current_token.type, context->current_token.line,
                                        NULL)) {
                parser_prec_stack_dispose(&stack);
                return parser_fail_internal(context, context->current_token.line);
            }
            load_token(context);
        } else if (relation == '<') {
            if (!parser_prec_stack_insert_handle_mark(&stack)) {
                parser_prec_stack_dispose(&stack);
                return parser_fail_internal(context, context->current_token.line);
            }

            if (current_prec_index == PARSER_PREC_ID) {
                int line = context->current_token.line;
                token_type_t tok_type = context->current_token.type;
                ast_node_t *operand_node = parse_operand(context);
                if (!operand_node) {
                    parser_prec_stack_dispose(&stack);
                    return NULL;
                }
                if (!parser_prec_stack_push(&stack, PARSER_PREC_STACK_ITEM_TERMINAL, PARSER_PREC_ID,
                                            tok_type, line, operand_node)) {
                    ast_dispose(&operand_node);
                    parser_prec_stack_dispose(&stack);
                    return parser_fail_internal(context, context->current_token.line);
                }
            } else {
                if (!parser_prec_stack_push(&stack, PARSER_PREC_STACK_ITEM_TERMINAL,
                                            current_prec_index, context->current_token.type,
                                            context->current_token.line, NULL)) {
                    parser_prec_stack_dispose(&stack);
                    return parser_fail_internal(context, context->current_token.line);
                }
                load_token(context);
            }
        } else if (relation == '>') {
            if (!parser_prec_table_reduce(context, &stack)) {
                parser_prec_stack_dispose(&stack);
                return NULL;
            }
        } else {
            parser_fail_syntax(context, context->current_token.line, "Syntax error in expression");
            parser_prec_stack_dispose(&stack);
            return NULL;
        }
    }
}
