/*
-------------------------------------------------------
Name of project: Translator IFJ26
Authors: Ondruška
Creation date: 23.10.2026
File: ast.c

Description: File with implementation of AST
-------------------------------------------------------
*/

#include "ast.h"
#include <stdlib.h>
#include <string.h>

// Helper function to create a new AST node with the specified type and source code line
static ast_node_t *ast_node_create(ast_node_type_t type, int source_code_line) {
    ast_node_t *new_node =
        calloc(1, sizeof(ast_node_t)); // calloc will set all the values to 0/NULL
    if (new_node == NULL) {
        return NULL;
    }

    new_node->source_code_line = source_code_line;
    new_node->type = type;
    return new_node;
}

// Function for creating ast node of type Block
ast_node_t *ast_block_create(int source_code_line) {
    return ast_node_create(AST_NODE_BLOCK, source_code_line);
}

// Function for adding statement to the end of block of code
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

// Function for creating ast node of type expression
ast_node_t *ast_expression_operation_create(ast_expression_operator_t op, ast_node_t *left_side,
                                            ast_node_t *right_side, int source_code_line) {

    ast_node_t *new_node = ast_node_create(AST_NODE_EXPRESSION_OPERATOR, source_code_line);
    if (new_node == NULL) {
        return NULL;
    }
    new_node->expression_operator.op = op;
    new_node->expression_operator.left_side = left_side;
    new_node->expression_operator.right_side = right_side;
    return new_node;
}

// Function for creating ast node of type variable declaration
ast_node_t *ast_var_declaration_create(char *name, int unique_id, symtable_datatype_t var_type,
                                       bool is_mutable, ast_node_t *init_expression,
                                       int source_code_line) {

    ast_node_t *new_node = ast_node_create(AST_NODE_VAR_DECLARATION, source_code_line);
    if (new_node == NULL) {
        return NULL;
    }
    new_node->var_declaration.is_mutable = is_mutable;
    new_node->var_declaration.name = name;
    new_node->var_declaration.unique_id = unique_id;
    new_node->var_declaration.var_type = var_type;
    new_node->var_declaration.init_expression = init_expression;
    return new_node;
}

// Function for creating ast node of type assignment
ast_node_t *ast_assignment_create(char *target_var_name, int unique_id, ast_node_t *expression,
                                  int source_code_line) {
    ast_node_t *new_node = ast_node_create(AST_NODE_VALUE_ASSIGNMENT, source_code_line);
    if (new_node == NULL) {
        return NULL;
    }
    new_node->assign_statement.expression = expression;
    new_node->assign_statement.target_var_name = target_var_name;
    new_node->assign_statement.unique_id = unique_id;
    return new_node;
}

// Function for creating ast node of type if-else statement
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

// Function for creating ast node of type while statement
ast_node_t *ast_while_create(ast_node_t *condition, ast_node_t *body_block, int source_code_line) {
    ast_node_t *new_node = ast_node_create(AST_NODE_STATEMENT_WHILE, source_code_line);
    if (new_node == NULL) {
        return NULL;
    }
    new_node->while_statement.body_block = body_block;
    new_node->while_statement.condition = condition;
    return new_node;
}

// Function for creating ast node of type expression variable
ast_node_t *ast_expression_var_create(char *name, int unique_id, symtable_datatype_t result_type,
                                      int source_code_line) {
    ast_node_t *new_node = ast_node_create(AST_NODE_EXPRESSION_VAR, source_code_line);
    if (new_node == NULL) {
        return NULL;
    }
    new_node->result_type = result_type;
    new_node->expression_var.name = name;
    new_node->expression_var.unique_id = unique_id;
    return new_node;
}

// Functions for creating ast nodes of type int constant
ast_node_t *ast_const_int_create(int value, int source_code_line) {
    ast_node_t *new_node = ast_node_create(AST_NODE_CONST_INT, source_code_line);
    if (new_node == NULL) {
        return NULL;
    }
    new_node->int_value = value;
    new_node->result_type = SYM_DATATYPE_INT;
    return new_node;
}

// Functions for creating ast nodes of type double constant
ast_node_t *ast_const_double_create(double value, int source_code_line) {
    ast_node_t *new_node = ast_node_create(AST_NODE_CONST_DOUBLE, source_code_line);
    if (new_node == NULL) {
        return NULL;
    }
    new_node->double_value = value;
    new_node->result_type = SYM_DATATYPE_DOUBLE;
    return new_node;
}

// Functions for creating ast nodes of type string constant
ast_node_t *ast_const_string_create(char *value, int source_code_line) {
    ast_node_t *new_node = ast_node_create(AST_NODE_CONST_STRING, source_code_line);
    if (new_node == NULL) {
        return NULL;
    }
    new_node->str_value = value;
    new_node->result_type = SYM_DATATYPE_STRING;
    return new_node;
}

// Functions for creating ast nodes of type unit constant
ast_node_t *ast_const_unit_create(int source_code_line) {
    ast_node_t *new_node = ast_node_create(AST_NODE_CONST_UNIT, source_code_line);
    if (new_node == NULL) {
        return NULL;
    }
    new_node->result_type = SYM_DATATYPE_UNIT;
    return new_node;
}

// Function for creating ast node of type function call expression
ast_node_t *ast_expression_fun_call_create(char *fun_name, ast_node_t *params, int params_count,
                                           int source_code_line) {
    ast_node_t *new_node = ast_node_create(AST_NODE_EXPRESSION_FUN_CALL, source_code_line);
    if (new_node == NULL) {
        return NULL;
    }
    new_node->expression_fun_call.fun_name = fun_name;
    new_node->expression_fun_call.params = params;
    new_node->expression_fun_call.params_count = params_count;
    return new_node;
}

// Function for creating ast node of type import statement
ast_node_t *ast_import_create(const char *module_name, int source_code_line) {
    ast_node_t *new_node = ast_node_create(AST_NODE_STATEMENT_IMPORT, source_code_line);
    if (new_node == NULL) {
        return NULL;
    }
    new_node->import_statement.module_name = module_name;
    return new_node;
}

// Function for creating ast node of type return statement
ast_node_t *ast_return_create(ast_node_t *expression, int source_code_line) {
    ast_node_t *new_node = ast_node_create(AST_NODE_STATEMENT_RETURN, source_code_line);
    if (new_node == NULL) {
        return NULL;
    }
    new_node->return_statement.expression = expression;
    return new_node;
}

// Helper function for creating ast node for function
static ast_node_t *ast_function_create(ast_node_type_t type, char *name,
                                       symtable_fun_param_t *params,
                                       symtable_datatype_t return_type, ast_node_t *body_block,
                                       int source_code_line) {
    ast_node_t *new_node = ast_node_create(type, source_code_line);
    if (new_node == NULL) {
        return NULL;
    }
    new_node->fun_definition.name = name;
    new_node->fun_definition.params = params;
    new_node->fun_definition.return_type = return_type;
    new_node->fun_definition.body_block = body_block;
    return new_node;
}

// Function for creating ast node of type function definition
ast_node_t *ast_fun_definition_create(char *name, symtable_fun_param_t *params,
                                      symtable_datatype_t return_type, ast_node_t *body_block,
                                      int source_code_line) {
    return ast_function_create(AST_NODE_FUN_DEFINITION, name, params, return_type, body_block,
                               source_code_line);
}

// Function for creating ast node of type main function
ast_node_t *ast_main_create(char *name, symtable_fun_param_t *params,
                            symtable_datatype_t return_type, ast_node_t *body_block,
                            int source_code_line) {
    return ast_function_create(AST_NODE_MAIN, name, params, return_type, body_block,
                               source_code_line);
}

// Function for disposing the AST tree
void ast_dispose(ast_node_t **root) {
    if (root == NULL || *root == NULL) {
        return;
    }

    ast_node_t *node = *root;

    if (node->next != NULL) {
        ast_dispose(&(node->next));
    }

    switch (node->type) {
    case AST_NODE_CONVERT:
    case AST_NODE_BLOCK:
        if (node->block.first_statement != NULL) {
            ast_dispose(&(node->block.first_statement));
        }
        break;

    case AST_NODE_STATEMENT_IMPORT:
        break;

    case AST_NODE_MAIN:
    case AST_NODE_FUN_DEFINITION:
        free(node->fun_definition.name);
        if (node->fun_definition.body_block != NULL) {
            ast_dispose(&(node->fun_definition.body_block));
        }
        {
            symtable_fun_param_t *param = node->fun_definition.params;
            while (param != NULL) {
                symtable_fun_param_t *next = param->next;
                free(param->name);
                free(param);
                param = next;
            }
        }
        break;

    case AST_NODE_VAR_DECLARATION:
        free(node->var_declaration.name);
        if (node->var_declaration.init_expression != NULL) {
            ast_dispose(&(node->var_declaration.init_expression));
        }
        break;

    case AST_NODE_VALUE_ASSIGNMENT:
        free(node->assign_statement.target_var_name);
        if (node->assign_statement.expression != NULL) {
            ast_dispose(&(node->assign_statement.expression));
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
        if (node->return_statement.expression != NULL) {
            ast_dispose(&(node->return_statement.expression));
        }
        break;

    case AST_NODE_EXPRESSION_OPERATOR:
        if (node->expression_operator.left_side != NULL) {
            ast_dispose(&(node->expression_operator.left_side));
        }
        if (node->expression_operator.right_side != NULL) {
            ast_dispose(&(node->expression_operator.right_side));
        }
        break;

    case AST_NODE_EXPRESSION_FUN_CALL:
        free(node->expression_fun_call.fun_name);
        if (node->expression_fun_call.params != NULL) {
            ast_dispose(&(node->expression_fun_call.params));
        }
        break;

    case AST_NODE_EXPRESSION_VAR:
        free(node->expression_var.name);
        break;
    case AST_NODE_CONST_INT:
    case AST_NODE_CONST_DOUBLE:
    case AST_NODE_CONST_UNIT:
        break;
    case AST_NODE_CONST_STRING:
        free(node->str_value);
        break;
    }

    free(node);
    *root = NULL;
}
