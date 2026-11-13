/*
-------------------------------------------------------
Name of project: Translator IFJ26
Authors: Zobal, Tajovský, Ondruška, Svoboda
Creation date: 23.10.2026
File: ast.c

Description: File with implementation of AST
-------------------------------------------------------
*/

#include "ast.h"
#include <stdlib.h>

ast_node_t *ast_node_create(ast_node_type_t type, int source_code_line) {
    ast_node_t *new_node =
        calloc(1, sizeof(ast_node_t)); // calloc will set all the values to 0/NULL
    if (new_node == NULL) {
        return NULL;
    } // not my problem :(

    new_node->source_code_line = source_code_line;
    new_node->type = type;
    return new_node;
}

ast_node_t *ast_block_create(ast_node_type_t type, int source_code_line) {
    if (type != AST_NODE_BLOCK) {
        return NULL;
    }
    return ast_node_create(type, source_code_line);
}

void ast_block_append(ast_node_t *block_node, ast_node_t *statement) {
    if (block_node == NULL || statement == NULL) {
        return;
    }
    if (block_node->block.first_statement == NULL) {
        block_node->block.first_statement = statement;
    } else {
        ast_node_t *current = block_node->block.first_statement;
        while (current->next != NULL) {
            current = current->next;
        }
        current->next = statement;
    }
    statement->next = NULL;
}

ast_node_t *ast_expres_operation_create(ast_expres_operator_t op, ast_node_t *left_side,
                                        ast_node_t *right_side, int source_code_line) {

    ast_node_t *new_node = ast_node_create(AST_NODE_EXPRES_OPERATOR, source_code_line);
    if (new_node == NULL) {
        return NULL;
    }
    new_node->expres_operator.op = op;
    new_node->expres_operator.left_side = left_side;
    new_node->expres_operator.right_side = right_side;
    return new_node;
}

ast_node_t *ast_var_declaration_create(const char *name, int unique_id,
                                       symtable_datatype_t var_type, bool is_mutable,
                                       ast_node_t *init_expres, int source_code_line) {

    ast_node_t *new_node = ast_node_create(AST_NODE_VAR_DECLARATION, source_code_line);
    if (new_node == NULL) {
        return NULL;
    }
    new_node->var_declaration.is_mutable = is_mutable;
    new_node->var_declaration.name = name;
    new_node->var_declaration.unique_id = unique_id;
    new_node->var_declaration.var_type = var_type;
    new_node->var_declaration.init_expres = init_expres;
    return new_node;
}

ast_node_t *ast_assignment_create(const char *target_var_name, int unique_id, ast_node_t *expres,
                                  int source_code_line) {
    ast_node_t *new_node = ast_node_create(AST_NODE_VALUE_ASSIGNMENT, source_code_line);
    if (new_node == NULL) {
        return NULL;
    }
    new_node->assign_statement.expres = expres;
    new_node->assign_statement.target_var_name = target_var_name;
    new_node->assign_statement.unique_id = unique_id;
    return new_node;
}

ast_node_t *ast_if_create(ast_node_t *condition, ast_node_t *if_block, ast_node_t *else_block,
                          int source_code_line) {
    ast_node_t *new_node = ast_node_create(AST_NODE_STATEMENT_IF, source_code_line);
    if (new_node == NULL) {
        return NULL;
    }
    new_node->if_statement.condition = condition;
    new_node->if_statement.else_block = else_block;
    new_node->if_statement.if_block = if_block;
    return new_node;
}

ast_node_t *ast_while_create(ast_node_t *condition, ast_node_t *body_block, int source_code_line) {
    ast_node_t *new_node = ast_node_create(AST_NODE_STATEMENT_WHILE, source_code_line);
    if (new_node == NULL) {
        return NULL;
    }
    new_node->while_statement.body_block = body_block;
    new_node->while_statement.condition = condition;
    return new_node;
}

ast_node_t *ast_expres_var_create(const char *name, int unique_id, symtable_datatype_t result_type,
                                  int source_code_line) {
    ast_node_t *new_node = ast_node_create(AST_NODE_EXPRES_VAR, source_code_line);
    if (new_node == NULL) {
        return NULL;
    }
    new_node->result_type = result_type;
    new_node->expressions_var.name = name;
    new_node->expressions_var.unique_id = unique_id;
    return new_node;
}

ast_node_t *ast_const_int_create(int value, int source_code_line) {
    ast_node_t *new_node = ast_node_create(AST_NODE_CONST_INT, source_code_line);
    if (new_node == NULL) {
        return NULL;
    }
    new_node->int_value = value;
    return new_node;
}
ast_node_t *ast_const_double_create(double value, int source_code_line) {
    ast_node_t *new_node = ast_node_create(AST_NODE_CONST_DOUBLE, source_code_line);
    if (new_node == NULL) {
        return NULL;
    }
    new_node->double_value = value;
    return new_node;
}

ast_node_t *ast_const_string_create(const char *value, int source_code_line) {
    ast_node_t *new_node = ast_node_create(AST_NODE_CONST_STRING, source_code_line);
    if (new_node == NULL) {
        return NULL;
    }
    new_node->str_value = value;
    return new_node;
}

ast_node_t *ast_const_unit_create(int source_code_line) {
    return ast_node_create(AST_NODE_CONST_UNIT, source_code_line);
}

ast_node_t *ast_expres_fun_call_create(char *fun_name, ast_node_t *params, int params_count,
                                       int source_code_line) {
    ast_node_t *new_node = ast_node_create(AST_NODE_EXPRES_FUN_CALL, source_code_line);
    if (new_node == NULL) {
        return NULL;
    }
    new_node->expres_fun_call.fun_name = fun_name;
    new_node->expres_fun_call.params = params;
    new_node->expres_fun_call.params_count = params_count;
    return new_node;
}

void ast_dispose(ast_node_t **root) {
    if (root == NULL || *root == NULL) {
        return;
    }

    ast_node_t *node = *root;

    if (node->next != NULL) {
        ast_dispose(&(node->next));
    }

    switch (node->type) {
    case AST_NODE_BLOCK:
        if (node->block.first_statement != NULL) {
            ast_dispose(&(node->block.first_statement));
        }
        break;

    case AST_NODE_STATEMENT_IMPORT:
        break;

    case AST_NODE_MAIN:
    case AST_NODE_FUN_DEFINITION:
        if (node->fun_definition.body_block != NULL) {
            ast_dispose(&(node->fun_definition.body_block));
        }
        {
            symtable_fun_param_t *param = node->fun_definition.params;
            while (param != NULL) {
                symtable_fun_param_t *next = param->next;
                free(param);
                param = next;
            }
        }
        break;

    case AST_NODE_VAR_DECLARATION:
        if (node->var_declaration.init_expres != NULL) {
            ast_dispose(&(node->var_declaration.init_expres));
        }
        break;

    case AST_NODE_VALUE_ASSIGNMENT:
        if (node->assign_statement.expres != NULL) {
            ast_dispose(&(node->assign_statement.expres));
        }
        break;

    case AST_NODE_STATEMENT_IF:
        if (node->if_statement.condition != NULL) {
            ast_dispose(&(node->if_statement.condition));
        }
        if (node->if_statement.if_block != NULL) {
            ast_dispose(&(node->if_statement.if_block));
        }
        if (node->if_statement.else_block != NULL) {
            ast_dispose(&(node->if_statement.else_block));
        }
        break;

    case AST_NODE_STATEMENT_WHILE:
        if (node->while_statement.condition != NULL) {
            ast_dispose(&(node->while_statement.condition));
        }
        if (node->while_statement.body_block != NULL) {
            ast_dispose(&(node->while_statement.body_block));
        }
        break;

    case AST_NODE_STATEMENT_RETURN:
        if (node->return_statement.expres != NULL) {
            ast_dispose(&(node->return_statement.expres));
        }
        break;

    case AST_NODE_EXPRES_OPERATOR:
        if (node->expres_operator.left_side != NULL) {
            ast_dispose(&(node->expres_operator.left_side));
        }
        if (node->expres_operator.right_side != NULL) {
            ast_dispose(&(node->expres_operator.right_side));
        }
        break;

    case AST_NODE_EXPRES_FUN_CALL:
        if (node->expres_fun_call.params != NULL) {
            ast_dispose(&(node->expres_fun_call.params));
        }
        break;

    case AST_NODE_EXPRES_VAR:
    case AST_NODE_CONST_INT:
    case AST_NODE_CONST_DOUBLE:
    case AST_NODE_CONST_STRING:
    case AST_NODE_CONST_UNIT:
        break;
    }

    free(node);
    *root = NULL;
}
