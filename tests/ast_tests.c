/*
-------------------------------------------------------
Name of project: Translator IFJ26
Authors: AI generated (Gemini), xsvoboj01 (Jakub Svoboda)
Creation date: 1.11.2026
File: ast_tests.c

Description: Unit tests for abstract syntax tree (AST).

-------------------------------------------------------
*/

#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>

#include <cmocka.h> // must be under std includes

#include "ast.h"

void ast_node_create_test(void **state) {
    (void)state;
    // Init
    ast_node_t *node = ast_node_create(AST_NODE_CONST_INT, 10);

    // Assert
    assert_non_null(node);
    assert_int_equal(node->type, AST_NODE_CONST_INT);
    assert_int_equal(node->source_code_line, 10);
    assert_null(node->next);

    ast_dispose(&node);
}

void ast_const_int_create_test(void **state) {
    (void)state;
    // Init
    ast_node_t *node = ast_const_int_create(42, 5);

    // Assert
    assert_non_null(node);
    assert_int_equal(node->type, AST_NODE_CONST_INT);
    assert_int_equal(node->int_value, 42);
    assert_int_equal(node->source_code_line, 5);

    ast_dispose(&node);
}

void ast_const_double_create_test(void **state) {
    (void)state;
    // Init
    ast_node_t *node = ast_const_double_create(3.14, 12);

    // Assert
    assert_non_null(node);
    assert_int_equal(node->type, AST_NODE_CONST_DOUBLE);
    assert_true(node->double_value == 3.14);
    assert_int_equal(node->source_code_line, 12);

    ast_dispose(&node);
}

void ast_const_string_create_test(void **state) {
    (void)state;
    // Init
    ast_node_t *node = ast_const_string_create("hello", 3);

    // Assert
    assert_non_null(node);
    assert_int_equal(node->type, AST_NODE_CONST_STRING);
    assert_string_equal(node->str_value, "hello");

    ast_dispose(&node);
}

void ast_block_create_and_append_test(void **state) {
    (void)state;
    // Init
    ast_node_t *block = ast_block_create(AST_NODE_BLOCK, 1);
    ast_node_t *stmt1 = ast_const_int_create(10, 2);
    ast_node_t *stmt2 = ast_const_int_create(20, 3);

    ast_block_append(block, stmt1);
    ast_block_append(block, stmt2);

    // Assert
    assert_non_null(block);
    assert_ptr_equal(block->block.first_statement, stmt1);
    assert_ptr_equal(stmt1->next, stmt2);
    assert_null(stmt2->next);

    ast_dispose(&block);
}

void ast_expres_operation_create_test(void **state) {
    (void)state;
    // Init
    ast_node_t *left = ast_const_int_create(1, 1);
    ast_node_t *right = ast_const_int_create(2, 1);
    ast_node_t *op_node = ast_expres_operation_create(AST_EXPRES_OPERATOR_ADD, left, right, 1);

    // Assert
    assert_non_null(op_node);
    assert_int_equal(op_node->type, AST_NODE_EXPRES_OPERATOR);
    assert_int_equal(op_node->expres_operator.op, AST_EXPRES_OPERATOR_ADD);
    assert_ptr_equal(op_node->expres_operator.left_side, left);
    assert_ptr_equal(op_node->expres_operator.right_side, right);

    ast_dispose(&op_node);
}

void ast_var_declaration_create_test(void **state) {
    (void)state;
    // Init
    ast_node_t *init_val = ast_const_int_create(10, 1);
    ast_node_t *var_decl = ast_var_declaration_create("x", 1, SYM_DATATYPE_INT, true, init_val, 1);

    // Assert
    assert_non_null(var_decl);
    assert_int_equal(var_decl->type, AST_NODE_VAR_DECLARATION);
    assert_string_equal(var_decl->var_declaration.name, "x");
    assert_true(var_decl->var_declaration.is_mutable);
    assert_ptr_equal(var_decl->var_declaration.init_expres, init_val);

    ast_dispose(&var_decl);
}

void ast_assignment_create_test(void **state) {
    (void)state;
    // Init
    ast_node_t *expr = ast_const_double_create(3.14, 2);
    ast_node_t *assign = ast_assignment_create("pi", 2, expr, 2);

    // Assert
    assert_non_null(assign);
    assert_int_equal(assign->type, AST_NODE_VALUE_ASSIGNMENT);
    assert_string_equal(assign->assign_statement.target_var_name, "pi");
    assert_ptr_equal(assign->assign_statement.expres, expr);

    ast_dispose(&assign);
}

void ast_if_create_test(void **state) {
    (void)state;
    // Init
    ast_node_t *cond = ast_const_int_create(1, 3);
    ast_node_t *if_block = ast_block_create(AST_NODE_BLOCK, 3);
    ast_node_t *else_block = ast_block_create(AST_NODE_BLOCK, 4);
    ast_node_t *if_node = ast_if_create(cond, if_block, else_block, 3);

    // Assert
    assert_non_null(if_node);
    assert_int_equal(if_node->type, AST_NODE_STATEMENT_IF);
    assert_ptr_equal(if_node->if_statement.condition, cond);
    assert_ptr_equal(if_node->if_statement.if_block, if_block);
    assert_ptr_equal(if_node->if_statement.else_block, else_block);

    ast_dispose(&if_node);
}

void ast_dispose_null_test(void **state) {
    (void)state;
    // Init
    ast_node_t *null_node = NULL;

    // Assert
    ast_dispose(&null_node);
    assert_null(null_node);
}
