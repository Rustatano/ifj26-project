/*
-------------------------------------------------------
Name of project: Translator IFJ26
File: test_ast.c

Description: Unit tests (cmocka) of AST node constructors,
             block appending and ast_dispose (ast.c).
             Memory ownership is verified with `make valgrind`.
-------------------------------------------------------
*/

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include "ast.h"
#include "test_helpers.h"
#include <stdlib.h>
#include <string.h>

#define T(fn) cmocka_unit_test_setup_teardown(fn, test_setup_timeout, test_teardown_timeout)

// ---------------------------------------------------------------------------
// Constructors
// ---------------------------------------------------------------------------

static void test_block_create(void **state) {
    (void)state;
    ast_node_t *b = ast_block_create(7);
    assert_non_null(b);
    assert_int_equal(b->type, AST_NODE_BLOCK);
    assert_int_equal(b->source_code_line, 7);
    assert_null(b->block.first_statement);
    assert_null(b->next);
    ast_dispose(&b);
}

static void test_const_int_create(void **state) {
    (void)state;
    ast_node_t *n = ast_const_int_create(-5, 3);
    assert_int_equal(n->type, AST_NODE_CONST_INT);
    assert_int_equal(n->int_value, -5);
    assert_int_equal(n->source_code_line, 3);
    ast_dispose(&n);
}

static void test_const_double_create(void **state) {
    (void)state;
    ast_node_t *n = ast_const_double_create(2.5, 1);
    assert_int_equal(n->type, AST_NODE_CONST_DOUBLE);
    assert_true(n->double_value == 2.5);
    ast_dispose(&n);
}

static void test_const_string_create_takes_ownership(void **state) {
    (void)state;
    char *s = strdup("text");
    ast_node_t *n = ast_const_string_create(s, 1);
    assert_int_equal(n->type, AST_NODE_CONST_STRING);
    assert_ptr_equal(n->str_value, s);
    ast_dispose(&n); // frees s
}

static void test_const_unit_create(void **state) {
    (void)state;
    ast_node_t *n = ast_const_unit_create(9);
    assert_int_equal(n->type, AST_NODE_CONST_UNIT);
    assert_int_equal(n->source_code_line, 9);
    ast_dispose(&n);
}

static void test_new_nodes_are_zero_initialized(void **state) {
    (void)state;
    ast_node_t *n = ast_block_create(1);
    assert_null(n->next);
    assert_int_equal(n->result_type, 0);
    ast_dispose(&n);
}

static void test_var_expression_create(void **state) {
    (void)state;
    ast_node_t *n = ast_expression_var_create(strdup("x"), 4, SYM_DATATYPE_ID, 2);
    assert_int_equal(n->type, AST_NODE_EXPRESSION_VAR);
    assert_string_equal(n->expression_var.name, "x");
    assert_int_equal(n->expression_var.unique_id, 4);
    assert_int_equal(n->result_type, SYM_DATATYPE_ID);
    ast_dispose(&n);
}

static void test_operation_create(void **state) {
    (void)state;
    ast_node_t *l = ast_const_int_create(1, 1);
    ast_node_t *r = ast_const_int_create(2, 1);
    ast_node_t *n = ast_expression_operation_create(AST_EXPRESSION_OPERATOR_ADD, l, r, 5);
    assert_int_equal(n->type, AST_NODE_EXPRESSION_OPERATOR);
    assert_int_equal(n->expression_operator.op, AST_EXPRESSION_OPERATOR_ADD);
    assert_ptr_equal(n->expression_operator.left_side, l);
    assert_ptr_equal(n->expression_operator.right_side, r);
    ast_dispose(&n); // disposes both operands
}

static void test_unary_operation_has_null_left_side(void **state) {
    (void)state;
    ast_node_t *r = ast_const_int_create(2, 1);
    ast_node_t *n =
        ast_expression_operation_create(AST_EXPRESSION_OPERATOR_UNARY_MINUS, NULL, r, 1);
    assert_null(n->expression_operator.left_side);
    assert_ptr_equal(n->expression_operator.right_side, r);
    ast_dispose(&n);
}

static void test_var_declaration_create(void **state) {
    (void)state;
    ast_node_t *init = ast_const_int_create(1, 1);
    ast_node_t *n = ast_var_declaration_create(strdup("v"), 3, SYM_DATATYPE_INT, true, init, 8);
    assert_int_equal(n->type, AST_NODE_VAR_DECLARATION);
    assert_string_equal(n->var_declaration.name, "v");
    assert_int_equal(n->var_declaration.unique_id, 3);
    assert_int_equal(n->var_declaration.var_type, SYM_DATATYPE_INT);
    assert_true(n->var_declaration.is_mutable);
    assert_ptr_equal(n->var_declaration.init_expression, init);
    ast_dispose(&n);
}

static void test_var_declaration_without_initializer_can_be_disposed(void **state) {
    (void)state;
    ast_node_t *n =
        ast_var_declaration_create(strdup("v"), 0, SYM_DATATYPE_UNKNOWN, false, NULL, 1);
    assert_null(n->var_declaration.init_expression);
    ast_dispose(&n);
}

static void test_assignment_create(void **state) {
    (void)state;
    ast_node_t *e = ast_const_int_create(1, 1);
    ast_node_t *n = ast_assignment_create(strdup("a"), 2, e, 6);
    assert_int_equal(n->type, AST_NODE_VALUE_ASSIGNMENT);
    assert_string_equal(n->assign_statement.target_var_name, "a");
    assert_int_equal(n->assign_statement.unique_id, 2);
    assert_ptr_equal(n->assign_statement.expression, e);
    ast_dispose(&n);
}

static void test_if_create_with_and_without_else(void **state) {
    (void)state;
    ast_node_t *n =
        ast_if_create(ast_const_int_create(1, 1), ast_block_create(1), ast_block_create(1), 2);
    assert_int_equal(n->type, AST_NODE_STATEMENT_IF);
    assert_non_null(n->if_statement.condition);
    assert_non_null(n->if_statement.if_block);
    assert_non_null(n->if_statement.else_block);
    ast_dispose(&n);

    n = ast_if_create(ast_const_int_create(1, 1), ast_block_create(1), NULL, 2);
    assert_null(n->if_statement.else_block);
    ast_dispose(&n);
}

static void test_while_create(void **state) {
    (void)state;
    ast_node_t *n = ast_while_create(ast_const_int_create(1, 1), ast_block_create(1), 4);
    assert_int_equal(n->type, AST_NODE_STATEMENT_WHILE);
    assert_non_null(n->while_statement.condition);
    assert_non_null(n->while_statement.body_block);
    ast_dispose(&n);
}

static void test_return_create_with_and_without_expression(void **state) {
    (void)state;
    ast_node_t *n = ast_return_create(ast_const_int_create(1, 1), 1);
    assert_int_equal(n->type, AST_NODE_STATEMENT_RETURN);
    assert_non_null(n->return_statement.expression);
    ast_dispose(&n);

    n = ast_return_create(NULL, 1);
    assert_null(n->return_statement.expression);
    ast_dispose(&n);
}

static void test_import_create_does_not_own_module_name(void **state) {
    (void)state;
    ast_node_t *n = ast_import_create("ifj26.*", 1);
    assert_int_equal(n->type, AST_NODE_STATEMENT_IMPORT);
    assert_string_equal(n->import_statement.module_name, "ifj26.*");
    ast_dispose(&n); // must not free the string literal
}

static void test_fun_call_create_stores_arguments(void **state) {
    (void)state;
    ast_node_t *a1 = ast_const_int_create(1, 1);
    ast_node_t *a2 = ast_const_int_create(2, 1);
    a1->next = a2;
    ast_node_t *n = ast_expression_fun_call_create(strdup("f"), a1, 2, 3);
    assert_int_equal(n->type, AST_NODE_EXPRESSION_FUN_CALL);
    assert_string_equal(n->expression_fun_call.fun_name, "f");
    assert_int_equal(n->expression_fun_call.params_count, 2);
    assert_ptr_equal(n->expression_fun_call.params, a1);
    ast_dispose(&n); // disposes the whole argument chain
}

static void test_fun_call_without_arguments(void **state) {
    (void)state;
    ast_node_t *n = ast_expression_fun_call_create(strdup("f"), NULL, 0, 1);
    assert_null(n->expression_fun_call.params);
    assert_int_equal(n->expression_fun_call.params_count, 0);
    ast_dispose(&n);
}

static symtable_fun_param_t *new_param(const char *name, symtable_datatype_t type) {
    symtable_fun_param_t *p = calloc(1, sizeof(*p));
    p->name = strdup(name);
    p->type = type;
    return p;
}

static void test_fun_definition_create(void **state) {
    (void)state;
    symtable_fun_param_t *p = new_param("a", SYM_DATATYPE_INT);
    p->next = new_param("b", SYM_DATATYPE_STRING);
    ast_node_t *n =
        ast_fun_definition_create(strdup("foo"), p, SYM_DATATYPE_INT, ast_block_create(1), 10);
    assert_int_equal(n->type, AST_NODE_FUN_DEFINITION);
    assert_string_equal(n->fun_definition.name, "foo");
    assert_ptr_equal(n->fun_definition.params, p);
    assert_int_equal(n->fun_definition.return_type, SYM_DATATYPE_INT);
    assert_non_null(n->fun_definition.body_block);
    ast_dispose(&n); // frees name, parameters (with names) and body
}

static void test_main_create_has_main_type(void **state) {
    (void)state;
    ast_node_t *n = ast_main_create(strdup("run"), NULL, SYM_DATATYPE_UNIT, ast_block_create(1), 1);
    assert_int_equal(n->type, AST_NODE_MAIN);
    assert_string_equal(n->fun_definition.name, "run");
    assert_int_equal(n->fun_definition.return_type, SYM_DATATYPE_UNIT);
    ast_dispose(&n);
}

// ---------------------------------------------------------------------------
// Block appending
// ---------------------------------------------------------------------------

static void test_block_append_first_statement(void **state) {
    (void)state;
    ast_node_t *b = ast_block_create(1);
    ast_node_t *s = ast_const_int_create(1, 1);
    ast_block_append(b, s);
    assert_ptr_equal(b->block.first_statement, s);
    assert_null(s->next);
    ast_dispose(&b);
}

static void test_block_append_keeps_order(void **state) {
    (void)state;
    ast_node_t *b = ast_block_create(1);
    ast_node_t *s1 = ast_const_int_create(1, 1);
    ast_node_t *s2 = ast_const_int_create(2, 1);
    ast_node_t *s3 = ast_const_int_create(3, 1);
    ast_block_append(b, s1);
    ast_block_append(b, s2);
    ast_block_append(b, s3);
    assert_ptr_equal(b->block.first_statement, s1);
    assert_ptr_equal(s1->next, s2);
    assert_ptr_equal(s2->next, s3);
    assert_null(s3->next);
    ast_dispose(&b);
}

static void test_block_append_null_arguments_are_ignored(void **state) {
    (void)state;
    ast_node_t *b = ast_block_create(1);
    ast_node_t *s = ast_const_int_create(1, 1);
    ast_block_append(NULL, s);
    ast_block_append(b, NULL);
    assert_null(b->block.first_statement);
    ast_dispose(&s);
    ast_dispose(&b);
}

// ---------------------------------------------------------------------------
// ast_dispose
// ---------------------------------------------------------------------------

static void test_dispose_null_is_safe(void **state) {
    (void)state;
    ast_node_t *n = NULL;
    ast_dispose(&n);
    ast_dispose(NULL);
    assert_null(n);
}

static void test_dispose_sets_pointer_to_null(void **state) {
    (void)state;
    ast_node_t *n = ast_const_int_create(1, 1);
    ast_dispose(&n);
    assert_null(n);
}

static void test_dispose_whole_program_tree(void **state) {
    (void)state;
    // import + @main def run(): Unit = { var x = 1 + 2; while x < 5 do { x = x + 1 }; if x == 5
    // then {..} else {..} }
    ast_node_t *root = ast_block_create(1);
    ast_block_append(root, ast_import_create("ifj26.*", 1));

    ast_node_t *body = ast_block_create(2);
    ast_block_append(body, ast_var_declaration_create(
                               strdup("x"), 0, SYM_DATATYPE_UNKNOWN, true,
                               ast_expression_operation_create(AST_EXPRESSION_OPERATOR_ADD,
                                                               ast_const_int_create(1, 2),
                                                               ast_const_int_create(2, 2), 2),
                               2));

    ast_node_t *loop_body = ast_block_create(3);
    ast_block_append(
        loop_body,
        ast_assignment_create(strdup("x"), 0,
                              ast_expression_operation_create(
                                  AST_EXPRESSION_OPERATOR_ADD,
                                  ast_expression_var_create(strdup("x"), 0, SYM_DATATYPE_ID, 3),
                                  ast_const_int_create(1, 3), 3),
                              3));
    ast_block_append(
        body, ast_while_create(ast_expression_operation_create(
                                   AST_EXPRESSION_OPERATOR_SMALLER,
                                   ast_expression_var_create(strdup("x"), 0, SYM_DATATYPE_ID, 3),
                                   ast_const_int_create(5, 3), 3),
                               loop_body, 3));

    ast_node_t *then_b = ast_block_create(4);
    ast_block_append(then_b, ast_expression_fun_call_create(
                                 strdup("print"), ast_const_string_create(strdup("yes"), 4), 1, 4));
    ast_node_t *else_b = ast_block_create(5);
    ast_block_append(else_b, ast_return_create(NULL, 5));
    ast_block_append(
        body, ast_if_create(ast_expression_operation_create(
                                AST_EXPRESSION_OPERATOR_EQUAL,
                                ast_expression_var_create(strdup("x"), 0, SYM_DATATYPE_ID, 4),
                                ast_const_int_create(5, 4), 4),
                            then_b, else_b, 4));

    ast_block_append(root, ast_main_create(strdup("run"), NULL, SYM_DATATYPE_UNIT, body, 2));
    ast_dispose(&root);
    assert_null(root);
}

static void test_dispose_conversion_node(void **state) {
    (void)state;
    // AST_NODE_CONVERT wraps its operand in block.first_statement
    ast_node_t *inner = ast_const_int_create(1, 1);
    ast_node_t *conv = ast_block_create(1);
    conv->type = AST_NODE_CONVERT;
    conv->block.first_statement = inner;
    ast_dispose(&conv);
}

int main(void) {
    const struct CMUnitTest tests[] = {
        T(test_block_create),
        T(test_const_int_create),
        T(test_const_double_create),
        T(test_const_string_create_takes_ownership),
        T(test_const_unit_create),
        T(test_new_nodes_are_zero_initialized),
        T(test_var_expression_create),
        T(test_operation_create),
        T(test_unary_operation_has_null_left_side),
        T(test_var_declaration_create),
        T(test_var_declaration_without_initializer_can_be_disposed),
        T(test_assignment_create),
        T(test_if_create_with_and_without_else),
        T(test_while_create),
        T(test_return_create_with_and_without_expression),
        T(test_import_create_does_not_own_module_name),
        T(test_fun_call_create_stores_arguments),
        T(test_fun_call_without_arguments),
        T(test_fun_definition_create),
        T(test_main_create_has_main_type),
        T(test_block_append_first_statement),
        T(test_block_append_keeps_order),
        T(test_block_append_null_arguments_are_ignored),
        T(test_dispose_null_is_safe),
        T(test_dispose_sets_pointer_to_null),
        T(test_dispose_whole_program_tree),
        T(test_dispose_conversion_node),
    };
    return cmocka_run_group_tests_name("ast", tests, NULL, NULL);
}
