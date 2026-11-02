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
#include "symbol_table.h"
#include <stdbool.h>
#include <stdlib.h>

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
ast_node_t *parse_statement(parser_context_t *context);

parse_result_t parse(ast_node_t **ast_root, scanner_conf_t *scanner_conf) {
    (void)ast_root;
    (void)scanner_conf;
    parse_result_t res = {0};
    return res;
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
        context->error_line = context->current_token.line;
        context->error_message = "Expected 'import' keyword";
        return NULL;
    }

    load_token(context);
    if (!verify_token_type(context, TOKEN_ID)) {
        context->error_line = context->current_token.line;
        context->error_message = "Expected module name identifier after import";
        return NULL;
    }
    char *module_name = context->current_token.attr.str_val;

    load_token(context);
    if (!verify_token_type(context, TOKEN_EOL)) {
        context->error_line = context->current_token.line;
        context->error_message = "Expected end of line after import statement";
        return NULL;
    }
    ast_node_t *new_node = ast_node_create(AST_NODE_STATEMENT_IMPORT, context->current_token.line);
    if (!new_node) {
        context->error_line = context->current_token.line;
        context->error_message = "Memory allocation failed for import AST node";
        return NULL;
    }
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

ast_node_t *parse_block(parser_context_t *context) {
    if (!verify_token_type(context, TOKEN_INDENT)) {
        context->error_line = context->current_token.line;
        context->error_message = "Expected indentation for block";
        return NULL;
    }
    int line = context->current_token.line;
    ast_node_t *block = ast_block_create(AST_NODE_BLOCK, line);
    if (!block) {
        context->error_line = line;
        context->error_message = "Memory allocation failed for block AST node";
        return NULL;
    }
    load_token(context);
    while (context->current_token.type != TOKEN_DEDENT) {
        ast_node_t *statement = parse_statement(context);
        if (statement == NULL) {
            ast_dispose(&block);
            return NULL;
        }
        ast_block_append(block, statement);
        load_token(context);
    }

    load_token(context);
    return block;
}

ast_node_t *parse_var_declaration(parser_context_t *context) {
    if (!(verify_token_type(context, TOKEN_KEYWORD_VAR) ||
          verify_token_type(context, TOKEN_KEYWORD_VAL))) {
        context->error_line = context->current_token.line;
        context->error_message = "Expected 'var' or 'val' keyword";
        return NULL;
    }
    bool is_mutable;
    if (context->current_token.type == TOKEN_KEYWORD_VAR) {
        is_mutable = false;
    } else if (context->current_token.type == TOKEN_KEYWORD_VAL) {
        is_mutable = true;
    } else {
        context->error_line = context->current_token.line;
        context->error_message = "Expected 'var' or 'val' keyword";
        return NULL;
    }
    int line = context->current_token.line;
    load_token(context);
    if (!verify_token_type(context, TOKEN_ID)) {
        context->error_line = context->current_token.line;
        context->error_message = "Expected variable name identifier";
        return NULL;
    }
    const char *var_id = context->current_token.attr.str_val;
    load_token(context);

    if (!verify_token_type(context, TOKEN_ASSIGN)) {
        context->error_line = context->current_token.line;
        context->error_message = "Expected '=' in variable declaration";
        return NULL;
    }
    load_token(context);

    ast_node_t *init_expres = parse_expresion(context);
    if (init_expres == NULL) {
        return NULL;
    }

    ast_node_t *var_node =
        ast_var_declaration_create(var_id, 0, SYM_DATATYPE_UNKNOWN, is_mutable, init_expres, line);
    if (!var_node) {
        context->error_line = line;
        context->error_message = "Memory allocation failed for variable declaration AST node";
        return NULL;
    }
    return var_node;
}

ast_node_t *parse_assignment(parser_context_t *context) {
    if (!verify_token_type(context, TOKEN_ID)) {
        context->error_line = context->current_token.line;
        context->error_message = "Expected target variable identifier in assignment";
        return NULL;
    }
    const char *target_var_name = context->current_token.attr.str_val;
    int line = context->current_token.line;
    load_token(context);

    if (!verify_token_type(context, TOKEN_ASSIGN)) {
        context->error_line = context->current_token.line;
        context->error_message = "Expected '=' in assignment";
        return NULL;
    }
    load_token(context);

    ast_node_t *expression = parse_expresion(context);
    if (expression == NULL) {
        return NULL;
    }

    ast_node_t *assign_node = ast_assignment_create(target_var_name, 0, expression, line);
    if (!assign_node) {
        context->error_line = line;
        context->error_message = "Memory allocation failed for assignment AST node";
        return NULL;
    }
    return assign_node;
}

ast_node_t *parse_return(parser_context_t *context) {
    if (!verify_token_type(context, TOKEN_KEYWORD_RETURN)) {
        context->error_line = context->current_token.line;
        context->error_message = "Expected 'return' keyword";
        return NULL;
    }

    int line = context->current_token.line;
    load_token(context);

    ast_node_t *expression = parse_expresion(context);
    if (expression == NULL) {
        return NULL;
    }

    ast_node_t *new_node = ast_node_create(AST_NODE_STATEMENT_RETURN, line);
    if (new_node == NULL) {
        context->error_line = line;
        context->error_message = "Memory allocation failed for return statement AST node";
        return NULL;
    }
    new_node->return_statement.expres = expression;

    return new_node;
}

symtable_datatype_t parse_data_type(parser_context_t *context) {
    if (verify_token_type(context, TOKEN_KEYWORD_INT)) {
        return SYM_DATATYPE_INT;
    } else if (verify_token_type(context, TOKEN_KEYWORD_DOUBLE)) {
        return SYM_DATATYPE_DOUBLE;
    } else if (verify_token_type(context, TOKEN_KEYWORD_STRING)) {
        return SYM_DATATYPE_STRING;
    } else if (verify_token_type(context, TOKEN_KEYWORD_UNIT)) {
        return SYM_DATATYPE_UNIT;
    }
    context->error_line = context->current_token.line;
    context->error_message = "Expected data type (Int, Double, String, or Unit)";
    return SYM_DATATYPE_UNKNOWN;
}

symtable_fun_param_t *parse_parameter(parser_context_t *context) {
    if (!verify_token_type(context, TOKEN_ID)) {
        context->error_line = context->current_token.line;
        context->error_message = "Expected parameter identifier";
        return NULL;
    }

    symtable_fun_param_t *param = calloc(1, sizeof(symtable_fun_param_t));
    if (!param) {
        context->error_line = context->current_token.line;
        context->error_message = "Memory allocation failed for function parameter";
        return NULL;
    }

    param->name = context->current_token.attr.str_val;
    load_token(context);

    if (!verify_token_type(context, TOKEN_COLON)) {
        context->error_line = context->current_token.line;
        context->error_message = "Expected ':' after parameter name";
        free(param);
        return NULL;
    }
    load_token(context);

    param->type = parse_data_type(context);
    if (param->type == SYM_DATATYPE_UNKNOWN) {
        free(param);
        return NULL;
    }
    load_token(context);
    return param;
}

ast_node_t *parse_fun_definition(parser_context_t *context) {
    if (!verify_token_type(context, TOKEN_KEYWORD_DEF)) {
        context->error_line = context->current_token.line;
        context->error_message = "Expected 'def' keyword";
        return NULL;
    }
    int line = context->current_token.line;

    load_token(context);
    if (!verify_token_type(context, TOKEN_ID)) {
        context->error_line = context->current_token.line;
        context->error_message = "Expected function name identifier";
        return NULL;
    }
    const char *fun_id = context->current_token.attr.str_val;
    load_token(context);

    if (!verify_token_type(context, TOKEN_LPAREN)) {
        context->error_line = context->current_token.line;
        context->error_message = "Expected '(' after function name";
        return NULL;
    }
    load_token(context);

    symtable_fun_param_t *head = NULL;
    symtable_fun_param_t *tail = NULL;

    if (!verify_token_type(context, TOKEN_RPAREN)) {
        while (true) {
            symtable_fun_param_t *param = parse_parameter(context);
            if (!param) {
                return NULL;
            }

            if (!head)
                head = param;
            else
                tail->next = param;
            tail = param;

            if (verify_token_type(context, TOKEN_RPAREN))
                break;

            if (!verify_token_type(context, TOKEN_COMMA)) {
                context->error_line = context->current_token.line;
                context->error_message = "Expected ',' between function parameters or ')'";
                return NULL;
            }
            load_token(context);
        }
    }
    load_token(context);

    symtable_datatype_t return_type = SYM_DATATYPE_UNIT;
    if (verify_token_type(context, TOKEN_COLON)) {
        load_token(context);
        return_type = parse_data_type(context);
        if (return_type == SYM_DATATYPE_UNKNOWN) {
            return NULL;
        }
        load_token(context);
    }

    ast_node_t *body_block = parse_block(context);
    if (!body_block) {
        context->error_line = context->current_token.line;
        context->error_message = "Expected function body block";
        return NULL;
    }

    ast_node_t *new_node = ast_node_create(AST_NODE_FUN_DEFINITION, line);
    if (!new_node) {
        context->error_line = line;
        context->error_message = "Memory allocation failed for function definition AST node";
        return NULL;
    }

    new_node->fun_definition.name = fun_id;
    new_node->fun_definition.params = head;
    new_node->fun_definition.return_type = return_type;
    new_node->fun_definition.body_block = body_block;

    return new_node;
}
