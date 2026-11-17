/*
-------------------------------------------------------
Name of project: Translator IFJ26
Authors: Ondruška
Creation date: 14.11.2026
File: parse_function.c

Description: Parsing of function definitions and returns
-------------------------------------------------------
*/

#include "parse_function.h"
#include "parse_block.h"
#include "parse_declaration.h"
#include "parse_expression.h"
#include <stdlib.h>

// Helper function for parsing one parametr with type
// Helper function for parsing one function parameter
// {param_id}: {DataType}
static symtable_fun_param_t *parse_parameter(parser_context_t *context) {
    int line = context->current_token.line;
    if (!expect_token(context, TOKEN_ID, "Expected parameter identifier")) {
        return NULL;
    }

    symtable_fun_param_t *param = calloc(1, sizeof(symtable_fun_param_t));
    if (param == NULL) {
        free(context->current_token.attr.str_val);
        return parser_fail_internal(context, line);
    }
    param->name = context->current_token.attr.str_val;
    load_token(context);

    if (!consume_token(context, TOKEN_COLON, "Expected ':' after parameter name")) {
        free(param->name);
        free(param);
        return NULL;
    }

    param->type = parse_data_type(context);
    if (param->type == SYM_DATATYPE_UNKNOWN) {
        free(param->name);
        free(param);
        return NULL;
    }
    load_token(context);
    return param;
}

// Helper function for parsing a list of parameters in a function definition
// Helper function for parsing a function parameter list
// {param_id}: {DataType}, ...
static bool parse_parameter_list(parser_context_t *context, symtable_fun_param_t **parameters) {
    symtable_fun_param_t *head = NULL;
    symtable_fun_param_t *tail = NULL;
    *parameters = NULL;

    skip_eols(context);
    if (check_token(context, TOKEN_RPAREN)) {
        load_token(context);
        return true;
    }

    while (true) {
        symtable_fun_param_t *parameter = parse_parameter(context);
        if (parameter == NULL) {
            dispose_parameters(&head);
            return false;
        }

        if (head == NULL) {
            head = parameter;
        } else {
            tail->next = parameter;
        }
        tail = parameter;

        if (check_token(context, TOKEN_RPAREN)) {
            load_token(context);
            *parameters = head;
            return true;
        }

        if (!consume_token(context, TOKEN_COMMA,
                           "Expected ',' between function parameters or ')'")) {
            dispose_parameters(&head);
            return false;
        }
        skip_eols(context);

        if (check_token(context, TOKEN_RPAREN)) {
            // Trailing comma is allowed only if a line break follows it (spec 1.2)
            if (!context->current_token.newline_before) {
                dispose_parameters(&head);
                parser_fail_syntax(context, context->current_token.line,
                                   "Trailing comma must be followed by a line break");
                return false;
            }
            load_token(context);
            *parameters = head;
            return true;
        }
    }
}

// Function for parsing a return statement with an optional expression
// return [{expression}]
ast_node_t *parse_return(parser_context_t *context) {
    int line = context->current_token.line;
    if (!consume_token(context, TOKEN_KW_RETURN, "Expected 'return' keyword")) {
        return NULL;
    }

    ast_node_t *expression = NULL;
    if (is_statement_terminator(context->current_token.type)) {
        expression = NULL;
    } else {
        expression = parse_expression(context);
        if (expression == NULL) {
            return NULL;
        }
    }

    ast_node_t *new_node = ast_return_create(expression, line);
    if (new_node == NULL) {
        ast_dispose(&expression);
        return parser_fail_internal(context, line);
    }
    return new_node;
}

// Function for parsing a single-line or multi-line function definition
// [@main]
// def {id}([{param_id}: {DataType}, ...]) [: {DataType}] = {expression}
//
// [@main]
// def {id}([{param_id}: {DataType}, ...]) [: {DataType}] =
//   {block of code}
ast_node_t *parse_fun_definition(parser_context_t *context, bool is_main) {
    int line = context->current_token.line;
    if (!consume_token(context, TOKEN_KW_DEF, "Expected 'def' keyword")) {
        return NULL;
    }
    if (!expect_token(context, TOKEN_ID, "Expected function name identifier")) {
        return NULL;
    }
    char *fun_id = context->current_token.attr.str_val;
    load_token(context);
    if (!consume_token(context, TOKEN_LPAREN, "Expected '(' after function name")) {
        free(fun_id);
        return NULL;
    }

    symtable_fun_param_t *param_list_head = NULL;
    if (!parse_parameter_list(context, &param_list_head)) {
        free(fun_id);
        return NULL;
    }

    symtable_datatype_t return_type = SYM_DATATYPE_UNKNOWN;
    if (check_token(context, TOKEN_COLON)) {
        load_token(context);
        return_type = parse_data_type(context);
        if (return_type == SYM_DATATYPE_UNKNOWN) {
            dispose_parameters(&param_list_head);
            free(fun_id);
            return NULL;
        }
        load_token(context);
    }

    if (!consume_token(context, TOKEN_ASSIGN, "Expected '=' after function header")) {
        dispose_parameters(&param_list_head);
        free(fun_id);
        return NULL;
    }

    ast_node_t *fun_body_node = NULL;
    if (check_token(context, TOKEN_EOL)) {
        fun_body_node = parse_block(context);
    } else {
        ast_node_t *expression = parse_expression(context);
        if (expression != NULL) {
            if (!is_statement_terminator(context->current_token.type)) {
                ast_dispose(&expression);
                dispose_parameters(&param_list_head);
                free(fun_id);
                parser_fail_syntax(context, context->current_token.line,
                                   "Expected end of one-line function");
                return NULL;
            }
            fun_body_node = ast_block_create(line);
            if (fun_body_node == NULL) {
                ast_dispose(&expression);
                dispose_parameters(&param_list_head);
                free(fun_id);
                return parser_fail_internal(context, line);
            }
            ast_block_append(fun_body_node, expression);
        }
    }
    if (fun_body_node == NULL) {
        dispose_parameters(&param_list_head);
        free(fun_id);
        return NULL;
    }

    ast_node_t *new_node =
        is_main
            ? ast_main_create(fun_id, param_list_head, return_type, fun_body_node, line)
            : ast_fun_definition_create(fun_id, param_list_head, return_type, fun_body_node, line);
    if (new_node == NULL) {
        ast_dispose(&fun_body_node);
        dispose_parameters(&param_list_head);
        free(fun_id);
        return parser_fail_internal(context, line);
    }

    return new_node;
}
