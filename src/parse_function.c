#include "parse_function.h"
#include "parse_block.h"
#include "parse_declaration.h"
#include "parse_expression.h"
#include <stdlib.h>

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
            load_token(context);
            *parameters = head;
            return true;
        }
    }
}

ast_node_t *parse_return(parser_context_t *context) {
    int line = context->current_token.line;
    if (!consume_token(context, TOKEN_KW_RETURN, "Expected 'return' keyword")) {
        return NULL;
    }

    ast_node_t *expression = NULL;
    if (is_statement_terminator(context->current_token.type)) {
        if (context->current_return_type != SYM_DATATYPE_UNIT) {
            parser_fail_syntax(context, line,
                               "A return without expression requires an explicit ': Unit' "
                               "return type");
            return NULL;
        }
        expression = ast_const_unit_create(line);
        if (expression == NULL) {
            return parser_fail_internal(context, line);
        }
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

    symtable_datatype_t previous_return_type = context->current_return_type;
    context->current_return_type = return_type;

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
                context->current_return_type = previous_return_type;
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
        context->current_return_type = previous_return_type;
        dispose_parameters(&param_list_head);
        free(fun_id);
        return NULL;
    }
    context->current_return_type = previous_return_type;

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
