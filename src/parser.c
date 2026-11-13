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
#include "parse_expresion.h"
#include "scanner.h"
#include "symbol_table.h"
#include <stdbool.h>
#include <stdlib.h>

// Forward declarations
ast_node_t *parse_statement(parser_context_t *context);
void load_token(parser_context_t *context);
bool check_token(parser_context_t *context, token_type_t token_type);

parse_result_t parse(ast_node_t **ast_root, scanner_conf_t *scanner_conf) {
    parse_result_t result = {
        .success = false, .error_code = PARSE_SUCCESS, .error_line = 0, .error_msg = NULL};

    if (ast_root == NULL || scanner_conf == NULL) {
        result.error_code = PARSE_ERR_INTERNAL;
        result.error_msg = "Invalid parameters passed to parse()";
        return result;
    }

    parser_context_t context = {.scanner_conf = scanner_conf,
                                .error_line = 0,
                                .error_message = NULL,
                                .error_code = PARSE_ERR_SYNTAX};

    load_token(&context);

    ast_node_t *root = ast_block_create(AST_NODE_MAIN, context.current_token.line);
    if (root == NULL) {
        result.error_code = PARSE_ERR_INTERNAL;
        result.error_msg = "Failed to allocate root AST node";
        return result;
    }

    while (!check_token(&context, TOKEN_EOF)) {
        while (check_token(&context, TOKEN_EOL)) {
            load_token(&context);
        }
        if (check_token(&context, TOKEN_EOF)) {
            break;
        }

        ast_node_t *stmt = parse_statement(&context);
        if (stmt == NULL) {
            ast_dispose(&root);
            *ast_root = NULL;

            result.success = false;
            result.error_code = context.error_code;
            result.error_line = context.error_line;
            result.error_msg = context.error_message;
            return result;
        }

        ast_block_append(root, stmt);

        if (check_token(&context, TOKEN_EOL)) {
            load_token(&context);
        }
    }

    *ast_root = root;
    result.success = true;
    result.error_code = PARSE_SUCCESS;
    return result;
}

void load_token(parser_context_t *context) {
    token_t loaded_token = get_token(context->scanner_conf);
    context->current_token = loaded_token;
}

bool check_token(parser_context_t *context, token_type_t token_type) {
    return context->current_token.type == token_type;
}

bool expect_token(parser_context_t *context, token_type_t token_type, const char *error_message) {
    if (context->current_token.type == token_type) {
        return true;
    }
    context->error_line = context->current_token.line;
    context->error_message = error_message;
    return false;
}

ast_node_t *parse_import(parser_context_t *context) {
    if (!expect_token(context, TOKEN_KW_IMPORT, "Expected 'import' keyword")) {
        return NULL;
    }

    load_token(context);
    if (!expect_token(context, TOKEN_ID, "Expected module name identifier after import")) {
        return NULL;
    }
    char *module_name = context->current_token.attr.str_val;

    load_token(context);
    if (!expect_token(context, TOKEN_EOL, "Expected end of line after import statement")) {
        return NULL;
    }
    ast_node_t *new_node = ast_node_create(AST_NODE_STATEMENT_IMPORT, context->current_token.line);
    if (!new_node) {
        context->error_line = context->current_token.line;
        context->error_message = "Memory allocation failed for import AST node";
        context->error_code = PARSE_ERR_INTERNAL;
        return NULL;
    }
    new_node->import_statement.module_name = module_name;

    return new_node;
}

ast_node_t *parse_block(parser_context_t *context) {
    while (check_token(context, TOKEN_EOL)) {
        load_token(context);
    }

    if (!expect_token(context, TOKEN_INDENT, "Expected indentation for block")) {
        return NULL;
    }
    int line = context->current_token.line;
    ast_node_t *block = ast_block_create(AST_NODE_BLOCK, line);
    if (!block) {
        context->error_line = line;
        context->error_message = "Memory allocation failed for block AST node";
        context->error_code = PARSE_ERR_INTERNAL;
        return NULL;
    }
    load_token(context);
    while (!check_token(context, TOKEN_DEDENT) && !check_token(context, TOKEN_EOF)) {
        while (check_token(context, TOKEN_EOL)) {
            load_token(context);
        }
        if (check_token(context, TOKEN_DEDENT) || check_token(context, TOKEN_EOF)) {
            break;
        }

        ast_node_t *statement = parse_statement(context);
        if (statement == NULL) {
            ast_dispose(&block);
            return NULL;
        }
        ast_block_append(block, statement);

        if (check_token(context, TOKEN_EOL)) {
            load_token(context);
        }
    }

    if (!expect_token(context, TOKEN_DEDENT, "Expected dedent at the end of block")) {
        ast_dispose(&block);
        return NULL;
    }
    load_token(context);
    return block;
}

ast_node_t *parse_var_declaration(parser_context_t *context) {
    if (!(check_token(context, TOKEN_KW_VAR) || check_token(context, TOKEN_KW_VAL))) {
        context->error_line = context->current_token.line;
        context->error_message = "Expected 'var' or 'val' KW";
        return NULL;
    }
    bool is_mutable = check_token(context, TOKEN_KW_VAR);
    int line = context->current_token.line;
    load_token(context);
    if (!expect_token(context, TOKEN_ID, "Expected variable name identifier")) {
        return NULL;
    }
    const char *var_id = context->current_token.attr.str_val;
    load_token(context);

    if (!expect_token(context, TOKEN_ASSIGN, "Expected '=' in variable declaration")) {
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
        context->error_code = PARSE_ERR_INTERNAL;
        return NULL;
    }
    return var_node;
}

ast_node_t *parse_assignment(parser_context_t *context) {
    if (!expect_token(context, TOKEN_ID, "Expected target variable identifier in assignment")) {
        return NULL;
    }
    const char *target_var_name = context->current_token.attr.str_val;
    int line = context->current_token.line;
    load_token(context);

    if (!expect_token(context, TOKEN_ASSIGN, "Expected '=' in assignment")) {
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
        context->error_code = PARSE_ERR_INTERNAL;
        return NULL;
    }
    return assign_node;
}

ast_node_t *parse_fun_call(parser_context_t *context) {
    if (!expect_token(context, TOKEN_ID, "Expected function name identifier")) {
        return NULL;
    }
    char *fun_name = context->current_token.attr.str_val;
    int line = context->current_token.line;
    load_token(context);

    if (!expect_token(context, TOKEN_LPAREN, "Expected '(' after function name")) {
        return NULL;
    }
    load_token(context);

    ast_node_t *params = NULL;
    ast_node_t *last_param = NULL;
    int params_count = 0;

    if (!check_token(context, TOKEN_RPAREN)) {
        while (true) {
            ast_node_t *arg = parse_expresion(context);
            if (!arg) {
                return NULL;
            }

            if (!params) {
                params = arg;
            } else {
                last_param->next = arg;
            }
            last_param = arg;
            params_count++;

            if (check_token(context, TOKEN_RPAREN))
                break;

            if (!expect_token(context, TOKEN_COMMA,
                              "Expected ',' or ')' in function call arguments")) {
                return NULL;
            }
            load_token(context);
        }
    }
    load_token(context);

    ast_node_t *fun_call = ast_expres_fun_call_create(fun_name, params, params_count, line);
    if (!fun_call) {
        context->error_line = line;
        context->error_message = "Memory allocation failed for function call AST node";
        context->error_code = PARSE_ERR_INTERNAL;
        return NULL;
    }
    return fun_call;
}

ast_node_t *parse_return(parser_context_t *context) {
    if (!expect_token(context, TOKEN_KW_RETURN, "Expected 'return' keyword")) {
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
        context->error_code = PARSE_ERR_INTERNAL;
        return NULL;
    }
    new_node->return_statement.expres = expression;

    return new_node;
}

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
    context->error_line = context->current_token.line;
    context->error_message = "Expected data type (Int, Double, String, or Unit)";
    return SYM_DATATYPE_UNKNOWN;
}

symtable_fun_param_t *parse_parameter(parser_context_t *context) {
    if (!expect_token(context, TOKEN_ID, "Expected parameter identifier")) {
        return NULL;
    }

    symtable_fun_param_t *param = calloc(1, sizeof(symtable_fun_param_t));
    if (!param) {
        context->error_line = context->current_token.line;
        context->error_message = "Memory allocation failed for function parameter";
        context->error_code = PARSE_ERR_INTERNAL;
        return NULL;
    }

    param->name = context->current_token.attr.str_val;
    load_token(context);

    if (!expect_token(context, TOKEN_COLON, "Expected ':' after parameter name")) {
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
    if (!expect_token(context, TOKEN_KW_DEF, "Expected 'def' keyword")) {
        return NULL;
    }
    int line = context->current_token.line;

    load_token(context);
    if (!expect_token(context, TOKEN_ID, "Expected function name identifier")) {
        return NULL;
    }
    const char *fun_id = context->current_token.attr.str_val;
    load_token(context);

    if (!expect_token(context, TOKEN_LPAREN, "Expected '(' after function name")) {
        return NULL;
    }
    load_token(context);

    symtable_fun_param_t *head = NULL;
    symtable_fun_param_t *tail = NULL;

    if (!check_token(context, TOKEN_RPAREN)) {
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

            if (check_token(context, TOKEN_RPAREN))
                break;

            if (!expect_token(context, TOKEN_COMMA,
                              "Expected ',' between function parameters or ')'")) {
                return NULL;
            }
            load_token(context);
        }
    }
    load_token(context);

    symtable_datatype_t return_type = SYM_DATATYPE_UNIT;
    if (check_token(context, TOKEN_COLON)) {
        load_token(context);
        return_type = parse_data_type(context);
        if (return_type == SYM_DATATYPE_UNKNOWN) {
            return NULL;
        }
        load_token(context);
    }

    ast_node_t *body_block = parse_block(context);
    if (!body_block) {
        return NULL;
    }

    ast_node_t *new_node = ast_node_create(AST_NODE_FUN_DEFINITION, line);
    if (!new_node) {
        context->error_line = line;
        context->error_message = "Memory allocation failed for function definition AST node";
        context->error_code = PARSE_ERR_INTERNAL;
        return NULL;
    }

    new_node->fun_definition.name = fun_id;
    new_node->fun_definition.params = head;
    new_node->fun_definition.return_type = return_type;
    new_node->fun_definition.body_block = body_block;

    return new_node;
}

ast_node_t *parse_if(parser_context_t *context) {
    if (!expect_token(context, TOKEN_KW_IF, "Expected 'if' keyword")) {
        return NULL;
    }
    int line = context->current_token.line;
    load_token(context);
    ast_node_t *condition = parse_expresion(context);
    if (condition == NULL) {
        return NULL;
    }
    if (!expect_token(context, TOKEN_KW_THEN, "Expected 'then' after if condition")) {
        return NULL;
    }
    load_token(context);
    ast_node_t *if_block = parse_block(context);
    if (if_block == NULL) {
        ast_dispose(&condition);
        return NULL;
    }
    ast_node_t *else_block = NULL;
    while (check_token(context, TOKEN_EOL)) {
        load_token(context);
    }
    if (check_token(context, TOKEN_KW_ELSE)) {
        load_token(context);
        else_block = parse_block(context);
        if (else_block == NULL) {
            ast_dispose(&condition);
            ast_dispose(&if_block);
            return NULL;
        }
    };
    ast_node_t *if_node = ast_if_create(condition, if_block, else_block, line);
    if (if_node == NULL) {
        context->error_line = line;
        context->error_message = "Memory allocation failed for if AST node";
        context->error_code = PARSE_ERR_INTERNAL;
        ast_dispose(&condition);
        ast_dispose(&if_block);
        ast_dispose(&else_block);
        return NULL;
    }
    return if_node;
}

ast_node_t *parse_while(parser_context_t *context) {
    if (!expect_token(context, TOKEN_KW_WHILE, "Expected 'while' keyword")) {
        return NULL;
    }
    int line = context->current_token.line;
    load_token(context);
    ast_node_t *condition = parse_expresion(context);
    if (condition == NULL) {
        return NULL;
    }
    if (!expect_token(context, TOKEN_KW_DO, "Expected 'do' after while condition")) {
        ast_dispose(&condition);
        return NULL;
    }
    load_token(context);
    ast_node_t *while_block = parse_block(context);
    if (while_block == NULL) {
        ast_dispose(&condition);
        return NULL;
    }
    ast_node_t *while_node = ast_while_create(condition, while_block, line);
    if (while_node == NULL) {
        context->error_line = line;
        context->error_message = "Memory allocation failed for while AST node";
        context->error_code = PARSE_ERR_INTERNAL;
        ast_dispose(&condition);
        ast_dispose(&while_block);
        return NULL;
    }
    return while_node;
}

ast_node_t *parse_statement(parser_context_t *context) {
    if (check_token(context, TOKEN_KW_VAR) || check_token(context, TOKEN_KW_VAL)) {
        return parse_var_declaration(context);
    }
    if (check_token(context, TOKEN_KW_IF)) {
        return parse_if(context);
    }
    if (check_token(context, TOKEN_KW_WHILE)) {
        return parse_while(context);
    }
    if (check_token(context, TOKEN_KW_RETURN)) {
        return parse_return(context);
    }
    if (check_token(context, TOKEN_KW_DEF)) {
        return parse_fun_definition(context);
    }
    if (check_token(context, TOKEN_KW_IMPORT)) {
        return parse_import(context);
    }

    if (check_token(context, TOKEN_ID)) {
        char *id_name = context->current_token.attr.str_val;
        int id_line = context->current_token.line;
        load_token(context);

        if (check_token(context, TOKEN_ASSIGN)) {
            load_token(context);
            ast_node_t *expr = parse_expresion(context);
            if (expr == NULL) {
                return NULL;
            }
            ast_node_t *assignment = ast_assignment_create(id_name, 0, expr, id_line);
            if (assignment == NULL) {
                context->error_line = id_line;
                context->error_message = "Memory allocation failed for assignment AST node";
                context->error_code = PARSE_ERR_INTERNAL;
                ast_dispose(&expr);
                return NULL;
            }
            return assignment;
        }

        if (check_token(context, TOKEN_LPAREN)) {
            load_token(context);
            ast_node_t *params = NULL;
            ast_node_t *last_param = NULL;
            int params_count = 0;

            if (!check_token(context, TOKEN_RPAREN)) {
                while (true) {
                    ast_node_t *arg = parse_expresion(context);
                    if (!arg) {
                        ast_dispose(&params);
                        return NULL;
                    }
                    if (!params) {
                        params = arg;
                    } else {
                        last_param->next = arg;
                    }
                    last_param = arg;
                    params_count++;

                    if (check_token(context, TOKEN_RPAREN))
                        break;
                    if (!expect_token(context, TOKEN_COMMA, "Expected ',' or ')'")) {
                        ast_dispose(&params);
                        return NULL;
                    }
                    load_token(context);
                }
            }
            load_token(context);
            ast_node_t *fun_call =
                ast_expres_fun_call_create(id_name, params, params_count, id_line);
            if (fun_call == NULL) {
                context->error_line = id_line;
                context->error_message = "Memory allocation failed for function call AST node";
                context->error_code = PARSE_ERR_INTERNAL;
                ast_dispose(&params);
                return NULL;
            }
            return fun_call;
        }

        context->error_line = context->current_token.line;
        context->error_message = "Expected '=' or '(' after identifier";
        return NULL;
    }

    context->error_line = context->current_token.line;
    context->error_message = "Unexpected token at start of statement";
    return NULL;
}
