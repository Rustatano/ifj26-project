/*
-------------------------------------------------------
Name of project: Translator IFJ26
Authors: Ondruška
Creation date: 14.11.2026
File: parse_declaration.c

Description: Parsing of variable declarations and assignments
-------------------------------------------------------
*/

#include "parse_declaration.h"
#include "parse_expression.h"
#include <stdlib.h>

// Function for converting token type to symbol table type
symtable_datatype_t parse_data_type(parser_context_t *context) {
    if (check_token(context, TOKEN_KW_INT)) {
        return SYM_DATATYPE_INT;
    } else if (check_token(context, TOKEN_KW_DOUBLE)) {
        return SYM_DATATYPE_DOUBLE;
    } else if (check_token(context, TOKEN_KW_STRING)) {
        return SYM_DATATYPE_STRING;
    } else if (check_token(context, TOKEN_KW_UNIT)) {
        return SYM_DATATYPE_UNIT;
    }
    parser_fail_syntax(context, context->current_token.line,
                       "Expected data type (Int, Double, String, or Unit)");
    return SYM_DATATYPE_UNKNOWN;
}

// Function for parsing a mutable (var) or immutable (val) variable declaration
// val {id} [: {DataType}] = {expression}
// var {id} [: {DataType}] = {expression}
ast_node_t *parse_var_declaration(parser_context_t *context) {
    int line = context->current_token.line;
    if (!(check_token(context, TOKEN_KW_VAR) || check_token(context, TOKEN_KW_VAL))) {
        return parser_fail_syntax(context, line, "Expected 'var' or 'val' KW");
    }
    bool is_mutable = check_token(context, TOKEN_KW_VAR);
    if (!load_token(context)) {
        return NULL;
    }

    if (!expect_token(context, TOKEN_ID, "Expected variable name identifier")) {
        return NULL;
    }
    char *var_id = context->current_token.attr.str_val;

    if (!load_token(context)) {
        return NULL;
    }

    symtable_datatype_t var_type = SYM_DATATYPE_UNKNOWN;
    if (check_token(context, TOKEN_COLON)) {
        if (!load_token(context)) {
            free(var_id);
            return NULL;
        }
        var_type = parse_data_type(context);
        if (var_type == SYM_DATATYPE_UNKNOWN) {
            free(var_id);
            return NULL;
        }
        if (!load_token(context)) {
            free(var_id);
            return NULL;
        }
    }

    if (!consume_token(context, TOKEN_ASSIGN, "Expected '=' in variable declaration")) {
        free(var_id);
        return NULL;
    }

    ast_node_t *init_expression_node = parse_expression(context);
    if (init_expression_node == NULL) {
        free(var_id);
        return NULL;
    }

    ast_node_t *var_node =
        ast_var_declaration_create(var_id, 0, var_type, is_mutable, init_expression_node, line);
    if (var_node == NULL) {
        free((void *)var_id);
        ast_dispose(&init_expression_node);
        return parser_fail_internal(context, line);
    }
    return var_node;
}

// Function for parsing a variable assignment statement
// {id} = {expression}
ast_node_t *parse_assignment(parser_context_t *context) {
    int line = context->current_token.line;
    if (!expect_token(context, TOKEN_ID, "Expected target variable identifier in assignment")) {
        return NULL;
    }
    char *target_var_name = context->current_token.attr.str_val;
    load_token(context);

    if (!consume_token(context, TOKEN_ASSIGN, "Expected '=' in assignment")) {
        free(target_var_name);
        return NULL;
    }

    ast_node_t *expression_node = parse_expression(context);
    if (expression_node == NULL) {
        free(target_var_name);
        return NULL;
    }

    ast_node_t *assignment_node = ast_assignment_create(target_var_name, 0, expression_node, line);
    if (assignment_node == NULL) {
        free(target_var_name);
        ast_dispose(&expression_node);
        return parser_fail_internal(context, line);
    }
    return assignment_node;
}
