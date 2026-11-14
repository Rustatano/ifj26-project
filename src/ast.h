/*
-------------------------------------------------------
Name of project: Translator IFJ26
Authors: Ondruška
Creation date: 14.10.2026
File: ast.h

Description: Header file for Abstract Syntax Tree (AST)
-------------------------------------------------------
*/

#ifndef AST_H
#define AST_H

#include "symbol_table.h"
#include <stdbool.h>
#include <stddef.h>

typedef struct ast_node ast_node_t;

typedef enum {
    // Program and structural nodes
    AST_NODE_MAIN,  // Used only in for main function
    AST_NODE_BLOCK, // Block of statements

    // Statements and declarations
    AST_NODE_FUN_DEFINITION,
    AST_NODE_VAR_DECLARATION,
    AST_NODE_VALUE_ASSIGNMENT,
    AST_NODE_STATEMENT_IF,
    AST_NODE_STATEMENT_WHILE,
    AST_NODE_STATEMENT_RETURN,
    AST_NODE_STATEMENT_IMPORT,

    // Expressions in source code
    AST_NODE_EXPRESSION_OPERATOR,
    AST_NODE_EXPRESSION_FUN_CALL,
    AST_NODE_EXPRESSION_VAR,

    // Constants in source code
    AST_NODE_CONST_INT,
    AST_NODE_CONST_DOUBLE,
    AST_NODE_CONST_STRING,
    AST_NODE_CONST_UNIT
} ast_node_type_t;

typedef enum {
    // Arithmetic
    AST_EXPRESSION_OPERATOR_ADD,
    AST_EXPRESSION_OPERATOR_SUB,
    AST_EXPRESSION_OPERATOR_MUL,
    AST_EXPRESSION_OPERATOR_DIV,
    AST_EXPRESSION_OPERATOR_UNARY_PLUS,
    AST_EXPRESSION_OPERATOR_UNARY_MINUS,

    // Logic
    AST_EXPRESSION_OPERATOR_EQUAL,         // ==
    AST_EXPRESSION_OPERATOR_NOT_EQUAL,     // !=
    AST_EXPRESSION_OPERATOR_SMALLER,       // <
    AST_EXPRESSION_OPERATOR_SMALLER_EQUAL, // <=
    AST_EXPRESSION_OPERATOR_BIGGER,        // >
    AST_EXPRESSION_OPERATOR_BIGGER_EQUAL   // >=
} ast_expression_operator_t;

struct ast_node {
    ast_node_type_t type;
    int source_code_line;
    symtable_datatype_t result_type;
    ast_node_t *next;

    // Type-specific values
    union {

        // AST_NODE_BLOCK
        struct {
            ast_node_t *first_statement;
        } block;

        // AST_NODE_STATEMENT_IMPORT
        struct {
            const char *module_name;
        } import_statement;

        // AST_NODE_FUN_DEFINITION
        struct {
            char *name;
            symtable_fun_param_t *params;
            symtable_datatype_t return_type;
            ast_node_t *body_block;
        } fun_definition;

        // AST_NODE_VAR_DECLARATION
        struct {
            char *name;
            int unique_id;
            symtable_datatype_t var_type;
            bool is_mutable;
            ast_node_t *init_expression; // Pointer to initialization  expression or NULL
        } var_declaration;

        // AST_NODE_VALUE_ASSIGNMENT
        struct {
            char *target_var_name;
            int unique_id;
            ast_node_t *expression; // Expression that gives the value
        } assign_statement;

        // AST_NODE_STATEMENT_IF
        struct {
            ast_node_t *condition;
            ast_node_t *if_block;
            ast_node_t *else_block; // Can be NULL
        } if_statement;

        // AST_NODE_STATEMENT_WHILE
        struct {
            ast_node_t *condition;
            ast_node_t *body_block;
        } while_statement;

        // AST_NODE_STATEMENT_RETURN
        struct {
            ast_node_t *expression;
        } return_statement;

        // AST_NODE_EXPRESSION_OPERATOR
        struct {
            ast_expression_operator_t op; // operator
            ast_node_t *left_side;
            ast_node_t *right_side;
        } expression_operator;

        // AST_NODE_EXPRESSION_FUN_CALL
        struct {
            char *fun_name;
            ast_node_t *params; // Linked list
            int params_count;
        } expression_fun_call;

        // AST_NODE_EXPRESSION_VAR
        struct {
            char *name;
            int unique_id;
        } expression_var;

        // Constants
        int int_value;
        double double_value;
        char *str_value;
    };
};

// Create block node
ast_node_t *ast_block_create(int source_code_line);

// Append statement to block statement list
void ast_block_append(ast_node_t *block_node, ast_node_t *statement);

// Create operation  expression node
ast_node_t *ast_expression_operation_create(ast_expression_operator_t op, ast_node_t *left_side,
                                            ast_node_t *right_side, int source_code_line);

// Create variable declaration node
ast_node_t *ast_var_declaration_create(char *name, int unique_id, symtable_datatype_t var_type,
                                       bool is_mutable, ast_node_t *init_expression,
                                       int source_code_line);

// Create variable assignment node
ast_node_t *ast_assignment_create(char *target_var_name, int unique_id, ast_node_t *expression,
                                  int source_code_line);

// Create if-else statement node
ast_node_t *ast_if_create(ast_node_t *condition, ast_node_t *if_block, ast_node_t *else_block,
                          int source_code_line);

// Create while loop statement node
ast_node_t *ast_while_create(ast_node_t *condition, ast_node_t *body_block, int source_code_line);

// Create variable  expression node
ast_node_t *ast_expression_var_create(char *name, int unique_id, symtable_datatype_t result_type,
                                      int source_code_line);

// Create constants
ast_node_t *ast_const_int_create(int value, int source_code_line);
ast_node_t *ast_const_double_create(double value, int source_code_line);
ast_node_t *ast_const_string_create(char *value, int source_code_line);
ast_node_t *ast_const_unit_create(int source_code_line);

// Create function call  expression node
ast_node_t *ast_expression_fun_call_create(char *fun_name, ast_node_t *params, int params_count,
                                           int source_code_line);

// Create statement nodes
ast_node_t *ast_import_create(const char *module_name, int source_code_line);
ast_node_t *ast_return_create(ast_node_t *expression, int source_code_line);

// Create function definition nodes
ast_node_t *ast_fun_definition_create(char *name, symtable_fun_param_t *params,
                                      symtable_datatype_t return_type, ast_node_t *body_block,
                                      int source_code_line);
ast_node_t *ast_main_create(char *name, symtable_fun_param_t *params,
                            symtable_datatype_t return_type, ast_node_t *body_block,
                            int source_code_line);

// Delete whole AST and free all memory
void ast_dispose(ast_node_t **root);

#endif
