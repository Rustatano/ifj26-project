/*
-------------------------------------------------------
Name of project: Translator IFJ26
File: test_semantic.c
Author: AI (Claude)

Description: Unit tests (cmocka) of the semantic analyzer (semantic.c).
             Part 1 tests individual check_* functions on hand-built
             AST nodes. Part 2 runs the complete pipeline
             (scanner -> parser -> semantic_analysis) on small programs
             in a forked child and compares the reported error code
             (read from stderr: "Semantic analysis Error <N>") with the
             codes required by the specification (section 6).
-------------------------------------------------------
*/

#define _GNU_SOURCE
#include "ast.h"
#include "parser.h"
#include "semantic.h"
#include "test_helpers.h"
#include <fcntl.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

#define PROLOG "import ifj26.*\n"
#define MAIN_HDR "@main def main(): Unit =\n"

// ---------------------------------------------------------------------------
// Helpers: silence stderr of the code under test (semantic errors are printed there)
// ---------------------------------------------------------------------------

static int saved_stderr = -1;

static void quiet_begin(void) {
    fflush(stderr);
    saved_stderr = dup(STDERR_FILENO);
    int devnull = open("/dev/null", O_WRONLY);
    dup2(devnull, STDERR_FILENO);
    close(devnull);
}

static void quiet_end(void) {
    fflush(stderr);
    dup2(saved_stderr, STDERR_FILENO);
    close(saved_stderr);
}

typedef struct {
    semantic_context_t ctx;
    symtable_stack_t stack;
} sem_fixture_t;

static int setup_fixture(void **state) {
    test_setup_timeout(state);
    sem_fixture_t *fx = calloc(1, sizeof(*fx));
    assert_true(semantic_init(&fx->ctx, &fx->stack));
    *state = fx;
    return 0;
}

static int teardown_fixture(void **state) {
    sem_fixture_t *fx = *state;
    symtable_stack_delete(&fx->stack);
    free(fx);
    test_teardown_timeout(state);
    return 0;
}

static ast_node_t *typed_int(int v) {
    ast_node_t *n = ast_const_int_create(v, 1);
    n->result_type = SYM_DATATYPE_INT;
    return n;
}

static ast_node_t *typed_double(double v) {
    ast_node_t *n = ast_const_double_create(v, 1);
    n->result_type = SYM_DATATYPE_DOUBLE;
    return n;
}

static ast_node_t *typed_string(const char *s) {
    ast_node_t *n = ast_const_string_create(strdup(s), 1);
    n->result_type = SYM_DATATYPE_STRING;
    return n;
}

static void declare_var(sem_fixture_t *fx, const char *name, symtable_datatype_t type,
                        bool is_mutable) {
    symtable_data_t d;
    memset(&d, 0, sizeof(d));
    d.type = SYM_TYPE_VAR;
    d.var.type = type;
    d.var.is_mutable = is_mutable;
    d.var.unique_id = fx->ctx.unique_var_id++;
    assert_true(symtable_insert_item(&fx->stack, (char *)name, d));
}

static symtable_fun_param_t *param(const char *name, symtable_datatype_t type,
                                   symtable_fun_param_t *next) {
    symtable_fun_param_t *p = calloc(1, sizeof(*p));
    p->name = strdup(name);
    p->type = type;
    p->next = next;
    return p;
}

static void declare_fun(sem_fixture_t *fx, const char *name, symtable_datatype_t ret,
                        symtable_fun_param_t *params) {
    symtable_data_t d;
    memset(&d, 0, sizeof(d));
    d.type = SYM_TYPE_FUNC;
    d.fun.return_type = ret;
    d.fun.params = params;
    d.fun.param_count = count_fun_params(NULL, params);
    assert_true(symtable_insert_item(&fx->stack, (char *)name, d));
}

// ---------------------------------------------------------------------------
// Part 1a: semantic_init, count_fun_params
// ---------------------------------------------------------------------------

static void test_semantic_init_null_stack_fails(void **state) {
    (void)state;
    semantic_context_t ctx = {0};
    quiet_begin();
    bool ok = semantic_init(&ctx, NULL);
    quiet_end();
    assert_false(ok);
}

static void test_semantic_init_creates_global_scope(void **state) {
    sem_fixture_t *fx = *state;
    assert_non_null(fx->stack.top);
    assert_ptr_equal(fx->stack.top, fx->stack.global);
}

static void test_count_fun_params(void **state) {
    (void)state;
    assert_int_equal(count_fun_params(NULL, NULL), 0);
    symtable_fun_param_t *p = param(
        "a", SYM_DATATYPE_INT, param("b", SYM_DATATYPE_INT, param("c", SYM_DATATYPE_INT, NULL)));
    assert_int_equal(count_fun_params(NULL, p), 3);
    assert_int_equal(count_fun_params(NULL, p->next->next), 1);
    while (p != NULL) {
        symtable_fun_param_t *next = p->next;
        free(p->name);
        free(p);
        p = next;
    }
}

// ---------------------------------------------------------------------------
// Part 1b: implicit conversion Int -> Double
// ---------------------------------------------------------------------------

static void test_create_conversion_node(void **state) {
    sem_fixture_t *fx = *state;
    ast_node_t *inner = typed_int(1);
    ast_node_t *conv = create_conversion_node(&fx->ctx, inner);
    assert_non_null(conv);
    assert_int_equal(conv->type, AST_NODE_CONVERT);
    assert_int_equal(conv->result_type, SYM_DATATYPE_DOUBLE);
    assert_ptr_equal(conv->block.first_statement, inner);
    assert_null(conv->next);
    ast_dispose(&conv);
}

static void test_conversion_of_int_constant_is_done_in_place(void **state) {
    sem_fixture_t *fx = *state;
    ast_node_t *n = typed_int(5);
    ast_node_t *orig = n;
    assert_int_equal(implicit_conversion_int_to_double(&fx->ctx, &n), SYM_DATATYPE_DOUBLE);
    assert_ptr_equal(n, orig);
    assert_int_equal(n->type, AST_NODE_CONST_DOUBLE);
    assert_true(n->double_value == 5.0);
    assert_int_equal(n->result_type, SYM_DATATYPE_DOUBLE);
    ast_dispose(&n);
}

static void test_conversion_of_int_expression_wraps_node(void **state) {
    sem_fixture_t *fx = *state;
    ast_node_t *n = ast_expression_var_create(strdup("x"), 0, SYM_DATATYPE_INT, 1);
    ast_node_t *orig = n;
    assert_int_equal(implicit_conversion_int_to_double(&fx->ctx, &n), SYM_DATATYPE_DOUBLE);
    assert_ptr_not_equal(n, orig);
    assert_int_equal(n->type, AST_NODE_CONVERT);
    assert_ptr_equal(n->block.first_statement, orig);
    ast_dispose(&n);
}

static void test_conversion_of_double_is_noop(void **state) {
    sem_fixture_t *fx = *state;
    ast_node_t *n = typed_double(1.5);
    ast_node_t *orig = n;
    assert_int_equal(implicit_conversion_int_to_double(&fx->ctx, &n), SYM_DATATYPE_DOUBLE);
    assert_ptr_equal(n, orig);
    assert_int_equal(n->type, AST_NODE_CONST_DOUBLE);
    ast_dispose(&n);
}

static void test_conversion_of_string_is_error(void **state) {
    sem_fixture_t *fx = *state;
    ast_node_t *n = typed_string("a");
    assert_int_equal(implicit_conversion_int_to_double(&fx->ctx, &n), SYM_DATATYPE_ERROR);
    ast_dispose(&n);
}

static void test_conversion_null_arguments(void **state) {
    sem_fixture_t *fx = *state;
    ast_node_t *n = NULL;
    assert_int_equal(implicit_conversion_int_to_double(&fx->ctx, NULL), SYM_DATATYPE_ERROR);
    assert_int_equal(implicit_conversion_int_to_double(&fx->ctx, &n), SYM_DATATYPE_ERROR);
}

// ---------------------------------------------------------------------------
// Part 1c: expressions
// ---------------------------------------------------------------------------

static void test_literal_nodes_carry_their_result_type(void **state) {
    (void)state;
    // check_expr() returns node->result_type for literals, so the node constructors
    // (or the parser) must set it. Otherwise every literal has type UNKNOWN (0).
    ast_node_t *i = ast_const_int_create(1, 1);
    ast_node_t *d = ast_const_double_create(1.5, 1);
    ast_node_t *s = ast_const_string_create(strdup("s"), 1);
    ast_node_t *u = ast_const_unit_create(1);
    assert_int_equal(i->result_type, SYM_DATATYPE_INT);
    assert_int_equal(d->result_type, SYM_DATATYPE_DOUBLE);
    assert_int_equal(s->result_type, SYM_DATATYPE_STRING);
    assert_int_equal(u->result_type, SYM_DATATYPE_UNIT);
    ast_dispose(&i);
    ast_dispose(&d);
    ast_dispose(&s);
    ast_dispose(&u);
}

static void test_check_expr_null_is_error(void **state) {
    sem_fixture_t *fx = *state;
    assert_int_equal(check_expr(&fx->ctx, &fx->stack, NULL), SYM_DATATYPE_ERROR);
}

static void test_check_expr_literals(void **state) {
    sem_fixture_t *fx = *state;
    ast_node_t *i = typed_int(1);
    ast_node_t *d = typed_double(1.5);
    ast_node_t *s = typed_string("s");
    ast_node_t *u = ast_const_unit_create(1);
    u->result_type = SYM_DATATYPE_UNIT;
    assert_int_equal(check_expr(&fx->ctx, &fx->stack, i), SYM_DATATYPE_INT);
    assert_int_equal(check_expr(&fx->ctx, &fx->stack, d), SYM_DATATYPE_DOUBLE);
    assert_int_equal(check_expr(&fx->ctx, &fx->stack, s), SYM_DATATYPE_STRING);
    assert_int_equal(check_expr(&fx->ctx, &fx->stack, u), SYM_DATATYPE_UNIT);
    ast_dispose(&i);
    ast_dispose(&d);
    ast_dispose(&s);
    ast_dispose(&u);
}

static void test_check_expr_invalid_node_type_is_error(void **state) {
    sem_fixture_t *fx = *state;
    ast_node_t *n = ast_block_create(1);
    quiet_begin();
    symtable_datatype_t t = check_expr(&fx->ctx, &fx->stack, n);
    quiet_end();
    assert_int_equal(t, SYM_DATATYPE_ERROR);
    ast_dispose(&n);
}

static void test_check_variable_expression(void **state) {
    sem_fixture_t *fx = *state;
    declare_var(fx, "x", SYM_DATATYPE_STRING, false);
    ast_node_t *use = ast_expression_var_create(strdup("x"), 0, SYM_DATATYPE_ID, 1);
    assert_int_equal(check_expres_var(&fx->ctx, &fx->stack, use), SYM_DATATYPE_STRING);
    ast_dispose(&use);
}

static void test_check_undefined_variable_is_error(void **state) {
    sem_fixture_t *fx = *state;
    ast_node_t *use = ast_expression_var_create(strdup("nope"), 0, SYM_DATATYPE_ID, 1);
    quiet_begin();
    symtable_datatype_t t = check_expres_var(&fx->ctx, &fx->stack, use);
    quiet_end();
    assert_int_equal(t, SYM_DATATYPE_ERROR);
    ast_dispose(&use);
}

static ast_node_t *binary(ast_expression_operator_t op, ast_node_t *l, ast_node_t *r) {
    return ast_expression_operation_create(op, l, r, 1);
}

static void test_operator_int_int(void **state) {
    sem_fixture_t *fx = *state;
    ast_node_t *e = binary(AST_EXPRESSION_OPERATOR_ADD, typed_int(1), typed_int(2));
    assert_int_equal(check_expres_operator(&fx->ctx, &fx->stack, e), SYM_DATATYPE_INT);
    ast_dispose(&e);
}

static void test_operator_double_double(void **state) {
    sem_fixture_t *fx = *state;
    ast_node_t *e = binary(AST_EXPRESSION_OPERATOR_MUL, typed_double(1), typed_double(2));
    assert_int_equal(check_expres_operator(&fx->ctx, &fx->stack, e), SYM_DATATYPE_DOUBLE);
    ast_dispose(&e);
}

static void test_operator_int_and_double_converts_int_side(void **state) {
    sem_fixture_t *fx = *state;
    ast_node_t *e = binary(AST_EXPRESSION_OPERATOR_ADD, typed_int(1), typed_double(2.5));
    assert_int_equal(check_expres_operator(&fx->ctx, &fx->stack, e), SYM_DATATYPE_DOUBLE);
    assert_int_equal(e->expression_operator.left_side->result_type, SYM_DATATYPE_DOUBLE);
    ast_dispose(&e);

    e = binary(AST_EXPRESSION_OPERATOR_SUB, typed_double(2.5), typed_int(1));
    assert_int_equal(check_expres_operator(&fx->ctx, &fx->stack, e), SYM_DATATYPE_DOUBLE);
    assert_int_equal(e->expression_operator.right_side->result_type, SYM_DATATYPE_DOUBLE);
    ast_dispose(&e);
}

static void test_operator_string_concatenation(void **state) {
    sem_fixture_t *fx = *state;
    ast_node_t *e = binary(AST_EXPRESSION_OPERATOR_ADD, typed_string("a"), typed_string("b"));
    assert_int_equal(check_expres_operator(&fx->ctx, &fx->stack, e), SYM_DATATYPE_STRING);
    ast_dispose(&e);
}

static void test_operator_string_and_int_is_error(void **state) {
    sem_fixture_t *fx = *state;
    ast_node_t *e = binary(AST_EXPRESSION_OPERATOR_ADD, typed_string("a"), typed_int(1));
    quiet_begin();
    symtable_datatype_t t = check_expres_operator(&fx->ctx, &fx->stack, e);
    quiet_end();
    assert_int_equal(t, SYM_DATATYPE_ERROR);
    ast_dispose(&e);
}

static void test_operator_minus_on_strings_is_error(void **state) {
    sem_fixture_t *fx = *state;
    // Only '+' is defined on strings (concatenation); '-', '*', '/' are numeric only.
    ast_node_t *e = binary(AST_EXPRESSION_OPERATOR_SUB, typed_string("a"), typed_string("b"));
    quiet_begin();
    symtable_datatype_t t = check_expres_operator(&fx->ctx, &fx->stack, e);
    quiet_end();
    assert_int_equal(t, SYM_DATATYPE_ERROR);
    ast_dispose(&e);
}

static void test_operator_error_in_operand_propagates(void **state) {
    sem_fixture_t *fx = *state;
    ast_node_t *bad = ast_expression_var_create(strdup("undefined"), 0, SYM_DATATYPE_ID, 1);
    ast_node_t *e = binary(AST_EXPRESSION_OPERATOR_ADD, typed_int(1), bad);
    quiet_begin();
    symtable_datatype_t t = check_expres_operator(&fx->ctx, &fx->stack, e);
    quiet_end();
    assert_int_equal(t, SYM_DATATYPE_ERROR);
    ast_dispose(&e);
}

static void test_operator_unary_minus_keeps_type(void **state) {
    sem_fixture_t *fx = *state;
    ast_node_t *e = ast_expression_operation_create(AST_EXPRESSION_OPERATOR_UNARY_MINUS, NULL,
                                                    typed_double(1.5), 1);
    symtable_datatype_t t = check_expr(&fx->ctx, &fx->stack, e);
    if (t != SYM_DATATYPE_DOUBLE) {
        print_error("unary minus on Double gave type %d, expected DOUBLE (left side is NULL)\n", t);
    }
    assert_int_equal(t, SYM_DATATYPE_DOUBLE);
    ast_dispose(&e);
}

static void test_unary_minus_on_string_is_error(void **state) {
    sem_fixture_t *fx = *state;
    ast_node_t *e = ast_expression_operation_create(AST_EXPRESSION_OPERATOR_UNARY_MINUS, NULL,
                                                    typed_string("a"), 1);
    quiet_begin();
    symtable_datatype_t t = check_expr(&fx->ctx, &fx->stack, e);
    quiet_end();
    assert_int_equal(t, SYM_DATATYPE_ERROR);
    ast_dispose(&e);
}

// ---------------------------------------------------------------------------
// Part 1d: function calls
// ---------------------------------------------------------------------------

static ast_node_t *call(const char *name, ast_node_t *args, int count) {
    return ast_expression_fun_call_create(strdup(name), args, count, 1);
}

static void test_call_undefined_function_is_error(void **state) {
    sem_fixture_t *fx = *state;
    ast_node_t *c = call("missing", NULL, 0);
    quiet_begin();
    symtable_datatype_t t = check_expres_fun_call(&fx->ctx, &fx->stack, c);
    quiet_end();
    assert_int_equal(t, SYM_DATATYPE_ERROR);
    ast_dispose(&c);
}

static void test_call_returns_function_return_type(void **state) {
    sem_fixture_t *fx = *state;
    declare_fun(fx, "f", SYM_DATATYPE_STRING, param("a", SYM_DATATYPE_INT, NULL));
    ast_node_t *c = call("f", typed_int(1), 1);
    assert_int_equal(check_expres_fun_call(&fx->ctx, &fx->stack, c), SYM_DATATYPE_STRING);
    ast_dispose(&c);
}

static void test_call_without_parameters(void **state) {
    sem_fixture_t *fx = *state;
    declare_fun(fx, "g", SYM_DATATYPE_INT, NULL);
    ast_node_t *c = call("g", NULL, 0);
    assert_int_equal(check_expres_fun_call(&fx->ctx, &fx->stack, c), SYM_DATATYPE_INT);
    ast_dispose(&c);
}

static void test_call_with_too_few_arguments(void **state) {
    sem_fixture_t *fx = *state;
    declare_fun(fx, "f", SYM_DATATYPE_INT,
                param("a", SYM_DATATYPE_INT, param("b", SYM_DATATYPE_INT, NULL)));
    ast_node_t *c = call("f", typed_int(1), 1);
    quiet_begin();
    symtable_datatype_t t = check_expres_fun_call(&fx->ctx, &fx->stack, c);
    quiet_end();
    assert_int_equal(t, SYM_DATATYPE_ERROR);
    ast_dispose(&c);
}

static void test_call_with_too_many_arguments(void **state) {
    sem_fixture_t *fx = *state;
    declare_fun(fx, "f", SYM_DATATYPE_INT, param("a", SYM_DATATYPE_INT, NULL));
    ast_node_t *a1 = typed_int(1);
    a1->next = typed_int(2);
    ast_node_t *c = call("f", a1, 2);
    quiet_begin();
    symtable_datatype_t t = check_expres_fun_call(&fx->ctx, &fx->stack, c);
    quiet_end();
    assert_int_equal(t, SYM_DATATYPE_ERROR);
    ast_dispose(&c);
}

static void test_call_with_wrong_argument_type(void **state) {
    sem_fixture_t *fx = *state;
    declare_fun(fx, "f", SYM_DATATYPE_INT, param("a", SYM_DATATYPE_INT, NULL));
    ast_node_t *c = call("f", typed_string("x"), 1);
    quiet_begin();
    symtable_datatype_t t = check_expres_fun_call(&fx->ctx, &fx->stack, c);
    quiet_end();
    assert_int_equal(t, SYM_DATATYPE_ERROR);
    ast_dispose(&c);
}

static void test_call_double_argument_for_int_parameter_is_error(void **state) {
    sem_fixture_t *fx = *state;
    declare_fun(fx, "f", SYM_DATATYPE_INT, param("a", SYM_DATATYPE_INT, NULL));
    ast_node_t *c = call("f", typed_double(1.5), 1);
    quiet_begin();
    symtable_datatype_t t = check_expres_fun_call(&fx->ctx, &fx->stack, c);
    quiet_end();
    assert_int_equal(t, SYM_DATATYPE_ERROR);
    ast_dispose(&c);
}

static void test_call_int_constant_for_double_parameter_is_converted(void **state) {
    sem_fixture_t *fx = *state;
    declare_fun(fx, "f", SYM_DATATYPE_INT, param("a", SYM_DATATYPE_DOUBLE, NULL));
    ast_node_t *c = call("f", typed_int(3), 1);
    assert_int_equal(check_expres_fun_call(&fx->ctx, &fx->stack, c), SYM_DATATYPE_INT);
    assert_int_equal(c->expression_fun_call.params->result_type, SYM_DATATYPE_DOUBLE);
    ast_dispose(&c);
}

static void test_call_int_variable_for_double_parameter_stays_linked(void **state) {
    sem_fixture_t *fx = *state;
    declare_fun(fx, "f", SYM_DATATYPE_INT,
                param("a", SYM_DATATYPE_DOUBLE, param("b", SYM_DATATYPE_INT, NULL)));
    declare_var(fx, "v", SYM_DATATYPE_INT, false);
    ast_node_t *arg1 = ast_expression_var_create(strdup("v"), 0, SYM_DATATYPE_ID, 1);
    arg1->next = typed_int(2);
    ast_node_t *c = call("f", arg1, 2);
    assert_int_equal(check_expres_fun_call(&fx->ctx, &fx->stack, c), SYM_DATATYPE_INT);
    // The first argument must now be the conversion node and the second one must still follow it.
    ast_node_t *first = c->expression_fun_call.params;
    assert_int_equal(first->type, AST_NODE_CONVERT);
    assert_non_null(first->next);
    assert_int_equal(first->next->type, AST_NODE_CONST_INT);
    ast_dispose(&c);
}

// ---------------------------------------------------------------------------
// Part 1e: declarations, assignment, return
// ---------------------------------------------------------------------------

static ast_node_t *decl(const char *name, symtable_datatype_t type, bool is_mutable,
                        ast_node_t *init) {
    return ast_var_declaration_create(strdup(name), 0, type, is_mutable, init, 1);
}

static void test_declaration_infers_type(void **state) {
    sem_fixture_t *fx = *state;
    ast_node_t *d = decl("x", SYM_DATATYPE_UNKNOWN, false, typed_string("s"));
    assert_int_equal(check_var_declaration(&fx->ctx, &fx->stack, d), SYM_DATATYPE_STRING);
    assert_int_equal(d->var_declaration.var_type, SYM_DATATYPE_STRING);
    symtable_node_t *n = symtable_search(&fx->stack, SYM_TYPE_VAR, "x");
    assert_non_null(n);
    assert_int_equal(n->data.var.type, SYM_DATATYPE_STRING);
    assert_false(n->data.var.is_mutable);
    ast_dispose(&d);
}

static void test_declaration_with_matching_explicit_type(void **state) {
    sem_fixture_t *fx = *state;
    ast_node_t *d = decl("x", SYM_DATATYPE_INT, true, typed_int(1));
    assert_int_equal(check_var_declaration(&fx->ctx, &fx->stack, d), SYM_DATATYPE_INT);
    assert_true(symtable_search(&fx->stack, SYM_TYPE_VAR, "x")->data.var.is_mutable);
    ast_dispose(&d);
}

static void test_declaration_int_to_double_conversion(void **state) {
    sem_fixture_t *fx = *state;
    ast_node_t *d = decl("x", SYM_DATATYPE_DOUBLE, false, typed_int(1));
    assert_int_equal(check_var_declaration(&fx->ctx, &fx->stack, d), SYM_DATATYPE_DOUBLE);
    assert_int_equal(d->var_declaration.init_expression->result_type, SYM_DATATYPE_DOUBLE);
    ast_dispose(&d);
}

static void test_declaration_type_mismatch_is_error(void **state) {
    sem_fixture_t *fx = *state;
    ast_node_t *d = decl("x", SYM_DATATYPE_INT, false, typed_string("s"));
    quiet_begin();
    symtable_datatype_t t = check_var_declaration(&fx->ctx, &fx->stack, d);
    quiet_end();
    assert_int_equal(t, SYM_DATATYPE_ERROR);
    ast_dispose(&d);
}

static void test_declaration_double_to_int_is_error(void **state) {
    sem_fixture_t *fx = *state;
    ast_node_t *d = decl("x", SYM_DATATYPE_INT, false, typed_double(1.5));
    quiet_begin();
    symtable_datatype_t t = check_var_declaration(&fx->ctx, &fx->stack, d);
    quiet_end();
    assert_int_equal(t, SYM_DATATYPE_ERROR);
    ast_dispose(&d);
}

static void test_declaration_redefinition_in_same_scope_is_error(void **state) {
    sem_fixture_t *fx = *state;
    ast_node_t *d1 = decl("x", SYM_DATATYPE_UNKNOWN, false, typed_int(1));
    ast_node_t *d2 = decl("x", SYM_DATATYPE_UNKNOWN, false, typed_int(2));
    assert_int_equal(check_var_declaration(&fx->ctx, &fx->stack, d1), SYM_DATATYPE_INT);
    quiet_begin();
    symtable_datatype_t t = check_var_declaration(&fx->ctx, &fx->stack, d2);
    quiet_end();
    assert_int_equal(t, SYM_DATATYPE_ERROR);
    ast_dispose(&d1);
    ast_dispose(&d2);
}

static void test_declaration_shadowing_in_inner_scope_is_ok(void **state) {
    sem_fixture_t *fx = *state;
    ast_node_t *d1 = decl("x", SYM_DATATYPE_UNKNOWN, false, typed_int(1));
    ast_node_t *d2 = decl("x", SYM_DATATYPE_UNKNOWN, false, typed_string("s"));
    assert_int_equal(check_var_declaration(&fx->ctx, &fx->stack, d1), SYM_DATATYPE_INT);
    assert_true(symtable_create_avl(&fx->stack));
    assert_int_equal(check_var_declaration(&fx->ctx, &fx->stack, d2), SYM_DATATYPE_STRING);
    ast_dispose(&d1);
    ast_dispose(&d2);
}

static void test_declaration_without_initializer_is_error(void **state) {
    sem_fixture_t *fx = *state;
    ast_node_t *d = decl("x", SYM_DATATYPE_INT, false, NULL);
    quiet_begin();
    symtable_datatype_t t = check_var_declaration(&fx->ctx, &fx->stack, d);
    quiet_end();
    assert_int_equal(t, SYM_DATATYPE_ERROR);
    ast_dispose(&d);
}

static void test_declarations_get_unique_ids(void **state) {
    sem_fixture_t *fx = *state;
    ast_node_t *d1 = decl("a", SYM_DATATYPE_UNKNOWN, false, typed_int(1));
    ast_node_t *d2 = decl("b", SYM_DATATYPE_UNKNOWN, false, typed_int(2));
    check_var_declaration(&fx->ctx, &fx->stack, d1);
    check_var_declaration(&fx->ctx, &fx->stack, d2);
    int id_a = symtable_search(&fx->stack, SYM_TYPE_VAR, "a")->data.var.unique_id;
    int id_b = symtable_search(&fx->stack, SYM_TYPE_VAR, "b")->data.var.unique_id;
    assert_int_not_equal(id_a, id_b);
    ast_dispose(&d1);
    ast_dispose(&d2);
}

static void test_declaration_unique_id_is_copied_to_ast(void **state) {
    sem_fixture_t *fx = *state;
    // codegen uses ast->var_declaration.unique_id for the variable name (name$id)
    ast_node_t *d1 = decl("a", SYM_DATATYPE_UNKNOWN, false, typed_int(1));
    ast_node_t *d2 = decl("b", SYM_DATATYPE_UNKNOWN, false, typed_int(2));
    check_var_declaration(&fx->ctx, &fx->stack, d1);
    check_var_declaration(&fx->ctx, &fx->stack, d2);
    assert_int_not_equal(d1->var_declaration.unique_id, d2->var_declaration.unique_id);
    ast_dispose(&d1);
    ast_dispose(&d2);
}

static ast_node_t *assign(const char *name, ast_node_t *expr) {
    return ast_assignment_create(strdup(name), 0, expr, 1);
}

static void test_assignment_ok(void **state) {
    sem_fixture_t *fx = *state;
    declare_var(fx, "x", SYM_DATATYPE_INT, true);
    ast_node_t *a = assign("x", typed_int(2));
    assert_int_equal(check_value_assignment(&fx->ctx, &fx->stack, a), SYM_DATATYPE_UNIT);
    ast_dispose(&a);
}

static void test_assignment_to_undefined_variable_is_error(void **state) {
    sem_fixture_t *fx = *state;
    ast_node_t *a = assign("x", typed_int(2));
    quiet_begin();
    symtable_datatype_t t = check_value_assignment(&fx->ctx, &fx->stack, a);
    quiet_end();
    assert_int_equal(t, SYM_DATATYPE_ERROR);
    ast_dispose(&a);
}

static void test_assignment_to_immutable_is_error(void **state) {
    sem_fixture_t *fx = *state;
    declare_var(fx, "x", SYM_DATATYPE_INT, false);
    ast_node_t *a = assign("x", typed_int(2));
    quiet_begin();
    symtable_datatype_t t = check_value_assignment(&fx->ctx, &fx->stack, a);
    quiet_end();
    assert_int_equal(t, SYM_DATATYPE_ERROR);
    ast_dispose(&a);
}

static void test_assignment_type_mismatch_is_error(void **state) {
    sem_fixture_t *fx = *state;
    declare_var(fx, "x", SYM_DATATYPE_INT, true);
    ast_node_t *a = assign("x", typed_string("s"));
    quiet_begin();
    symtable_datatype_t t = check_value_assignment(&fx->ctx, &fx->stack, a);
    quiet_end();
    assert_int_equal(t, SYM_DATATYPE_ERROR);
    ast_dispose(&a);
}

static void test_assignment_int_to_double_variable_is_converted(void **state) {
    sem_fixture_t *fx = *state;
    declare_var(fx, "x", SYM_DATATYPE_DOUBLE, true);
    ast_node_t *a = assign("x", typed_int(2));
    assert_int_equal(check_value_assignment(&fx->ctx, &fx->stack, a), SYM_DATATYPE_UNIT);
    assert_int_equal(a->assign_statement.expression->result_type, SYM_DATATYPE_DOUBLE);
    ast_dispose(&a);
}

static void test_assignment_double_to_int_variable_is_error(void **state) {
    sem_fixture_t *fx = *state;
    declare_var(fx, "x", SYM_DATATYPE_INT, true);
    ast_node_t *a = assign("x", typed_double(1.5));
    quiet_begin();
    symtable_datatype_t t = check_value_assignment(&fx->ctx, &fx->stack, a);
    quiet_end();
    assert_int_equal(t, SYM_DATATYPE_ERROR);
    ast_dispose(&a);
}

static void test_assignment_copies_unique_id_to_ast(void **state) {
    sem_fixture_t *fx = *state;
    declare_var(fx, "pad", SYM_DATATYPE_INT, true);
    declare_var(fx, "x", SYM_DATATYPE_INT, true);
    int expected = symtable_search(&fx->stack, SYM_TYPE_VAR, "x")->data.var.unique_id;
    ast_node_t *a = assign("x", typed_int(2));
    check_value_assignment(&fx->ctx, &fx->stack, a);
    assert_int_equal(a->assign_statement.unique_id, expected);
    ast_dispose(&a);
}

static symtable_node_t *enter_function(sem_fixture_t *fx, const char *name,
                                       symtable_datatype_t ret) {
    declare_fun(fx, name, ret, NULL);
    return symtable_search(&fx->stack, SYM_TYPE_FUNC, (char *)name);
}

static void test_return_matching_type(void **state) {
    sem_fixture_t *fx = *state;
    symtable_node_t *f = enter_function(fx, "f", SYM_DATATYPE_INT);
    ast_node_t *r = ast_return_create(typed_int(1), 1);
    assert_int_equal(check_statement_return(&fx->ctx, &fx->stack, r, f), SYM_DATATYPE_INT);
    ast_dispose(&r);
}

static void test_return_int_in_double_function_is_converted(void **state) {
    sem_fixture_t *fx = *state;
    symtable_node_t *f = enter_function(fx, "f", SYM_DATATYPE_DOUBLE);
    ast_node_t *r = ast_return_create(typed_int(1), 1);
    symtable_datatype_t t = check_statement_return(&fx->ctx, &fx->stack, r, f);
    assert_int_not_equal(t, SYM_DATATYPE_ERROR);
    assert_int_equal(r->return_statement.expression->result_type, SYM_DATATYPE_DOUBLE);
    ast_dispose(&r);
}

static void test_return_wrong_type_is_error(void **state) {
    sem_fixture_t *fx = *state;
    symtable_node_t *f = enter_function(fx, "f", SYM_DATATYPE_INT);
    ast_node_t *r = ast_return_create(typed_string("s"), 1);
    quiet_begin();
    symtable_datatype_t t = check_statement_return(&fx->ctx, &fx->stack, r, f);
    quiet_end();
    assert_int_equal(t, SYM_DATATYPE_ERROR);
    ast_dispose(&r);
}

static void test_bare_return_in_unit_function_is_ok(void **state) {
    sem_fixture_t *fx = *state;
    // spec 2.3 rule 1: a bare 'return' returns () and is allowed in a function with return type
    // Unit
    symtable_node_t *f = enter_function(fx, "f", SYM_DATATYPE_UNIT);
    ast_node_t *r = ast_return_create(NULL, 1);
    quiet_begin();
    symtable_datatype_t t = check_statement_return(&fx->ctx, &fx->stack, r, f);
    quiet_end();
    assert_int_not_equal(t, SYM_DATATYPE_ERROR);
    ast_dispose(&r);
}

static void test_return_value_in_unit_function_is_discarded_not_error(void **state) {
    sem_fixture_t *fx = *state;
    // spec 2.3 rule 4: in a function with explicit return type Unit the value is evaluated and
    // dropped
    symtable_node_t *f = enter_function(fx, "f", SYM_DATATYPE_UNIT);
    ast_node_t *r = ast_return_create(typed_int(1), 1);
    quiet_begin();
    symtable_datatype_t t = check_statement_return(&fx->ctx, &fx->stack, r, f);
    quiet_end();
    assert_int_not_equal(t, SYM_DATATYPE_ERROR);
    ast_dispose(&r);
}

// ---------------------------------------------------------------------------
// Part 1f: blocks and statements
// ---------------------------------------------------------------------------

static void test_check_block_null_is_unit(void **state) {
    sem_fixture_t *fx = *state;
    assert_int_equal(check_block(&fx->ctx, &fx->stack, NULL, NULL), SYM_DATATYPE_UNIT);
}

static void test_check_block_empty(void **state) {
    sem_fixture_t *fx = *state;
    ast_node_t *b = ast_block_create(1);
    assert_int_equal(check_block(&fx->ctx, &fx->stack, b, NULL), SYM_DATATYPE_UNIT);
    ast_dispose(&b);
}

static void test_check_block_scope_is_popped(void **state) {
    sem_fixture_t *fx = *state;
    ast_node_t *b = ast_block_create(1);
    ast_block_append(b, decl("local", SYM_DATATYPE_UNKNOWN, false, typed_int(1)));
    assert_int_equal(check_block(&fx->ctx, &fx->stack, b, NULL), SYM_DATATYPE_UNIT);
    assert_null(symtable_search(&fx->stack, SYM_TYPE_VAR, "local"));
    assert_ptr_equal(fx->stack.top, fx->stack.global);
    ast_dispose(&b);
}

static void test_check_block_sees_outer_variables(void **state) {
    sem_fixture_t *fx = *state;
    declare_var(fx, "outer", SYM_DATATYPE_INT, true);
    ast_node_t *b = ast_block_create(1);
    ast_block_append(b, assign("outer", typed_int(1)));
    assert_int_equal(check_block(&fx->ctx, &fx->stack, b, NULL), SYM_DATATYPE_UNIT);
    ast_dispose(&b);
}

static void test_check_block_error_stops_and_restores_scope(void **state) {
    sem_fixture_t *fx = *state;
    ast_node_t *b = ast_block_create(1);
    ast_block_append(b, decl("x", SYM_DATATYPE_INT, false, typed_string("bad")));
    quiet_begin();
    symtable_datatype_t t = check_block(&fx->ctx, &fx->stack, b, NULL);
    quiet_end();
    assert_int_equal(t, SYM_DATATYPE_ERROR);
    assert_ptr_equal(fx->stack.top, fx->stack.global);
    ast_dispose(&b);
}

static void test_check_block_expression_statements(void **state) {
    sem_fixture_t *fx = *state;
    // A block may contain stand-alone expressions (e.g. the value of the function) - spec 2.1
    ast_node_t *b = ast_block_create(1);
    ast_block_append(b, typed_int(1));
    quiet_begin();
    symtable_datatype_t t = check_block(&fx->ctx, &fx->stack, b, NULL);
    quiet_end();
    assert_int_not_equal(t, SYM_DATATYPE_ERROR);
    ast_dispose(&b);
}

static void test_check_block_variable_expression_statement(void **state) {
    sem_fixture_t *fx = *state;
    declare_var(fx, "x", SYM_DATATYPE_INT, false);
    ast_node_t *b = ast_block_create(1);
    ast_block_append(b, ast_expression_var_create(strdup("x"), 0, SYM_DATATYPE_ID, 1));
    quiet_begin();
    symtable_datatype_t t = check_block(&fx->ctx, &fx->stack, b, NULL);
    quiet_end();
    assert_int_not_equal(t, SYM_DATATYPE_ERROR);
    ast_dispose(&b);
}

static void test_if_checks_both_branches(void **state) {
    sem_fixture_t *fx = *state;
    ast_node_t *then_b = ast_block_create(1);
    ast_node_t *else_b = ast_block_create(1);
    ast_block_append(else_b, decl("x", SYM_DATATYPE_INT, false, typed_string("bad")));
    ast_node_t *cond = binary(AST_EXPRESSION_OPERATOR_SMALLER, typed_int(1), typed_int(2));
    ast_node_t *i = ast_if_create(cond, then_b, else_b, 1);
    quiet_begin();
    symtable_datatype_t t = check_statement_if(&fx->ctx, &fx->stack, i, NULL);
    quiet_end();
    assert_int_equal(t, SYM_DATATYPE_ERROR);
    ast_dispose(&i);
}

static void test_if_condition_must_be_relational(void **state) {
    sem_fixture_t *fx = *state;
    // spec 2.4: the condition must be a relational expression (Boolean)
    ast_node_t *i = ast_if_create(typed_int(1), ast_block_create(1), ast_block_create(1), 1);
    quiet_begin();
    symtable_datatype_t t = check_statement_if(&fx->ctx, &fx->stack, i, NULL);
    quiet_end();
    assert_int_equal(t, SYM_DATATYPE_ERROR);
    ast_dispose(&i);
}

static void test_while_condition_must_be_relational(void **state) {
    sem_fixture_t *fx = *state;
    ast_node_t *w = ast_while_create(typed_string("x"), ast_block_create(1), 1);
    quiet_begin();
    symtable_datatype_t t = check_statement_while(&fx->ctx, &fx->stack, w, NULL);
    quiet_end();
    assert_int_equal(t, SYM_DATATYPE_ERROR);
    ast_dispose(&w);
}

static void test_while_with_relational_condition_is_ok(void **state) {
    sem_fixture_t *fx = *state;
    declare_var(fx, "i", SYM_DATATYPE_INT, true);
    ast_node_t *cond =
        binary(AST_EXPRESSION_OPERATOR_SMALLER,
               ast_expression_var_create(strdup("i"), 0, SYM_DATATYPE_ID, 1), typed_int(3));
    ast_node_t *w = ast_while_create(cond, ast_block_create(1), 1);
    assert_int_equal(check_statement_while(&fx->ctx, &fx->stack, w, NULL), SYM_DATATYPE_UNIT);
    ast_dispose(&w);
}

// ---------------------------------------------------------------------------
// Part 1g: two AST passes
// ---------------------------------------------------------------------------

static void test_first_pass_null_arguments(void **state) {
    sem_fixture_t *fx = *state;
    assert_false(first_ast_passage(&fx->ctx, NULL, &fx->stack));
    ast_node_t *b = ast_block_create(1);
    assert_false(first_ast_passage(&fx->ctx, b, NULL));
    ast_dispose(&b);
}

static void test_second_pass_null_arguments(void **state) {
    sem_fixture_t *fx = *state;
    assert_false(second_ast_passage(&fx->ctx, NULL, &fx->stack));
    ast_node_t *b = ast_block_create(1);
    assert_false(second_ast_passage(&fx->ctx, b, NULL));
    ast_dispose(&b);
}

static void test_semantic_analysis_null_arguments(void **state) {
    (void)state;
    symtable_stack_t st;
    quiet_begin();
    assert_false(semantic_analysis(NULL, &st));
    quiet_end();
}

// ---------------------------------------------------------------------------
// Part 2: whole pipeline on source text, exit code read from stderr
// ---------------------------------------------------------------------------

// Returns 0 = semantically OK, N>0 = reported error code N, -1 = parser rejected the program,
// -2 = crash / timeout / hang, -3 = analysis failed without printing a code.
static int analyze_source(const char *text) {
    int pipefd[2];
    assert_int_equal(pipe(pipefd), 0);
    fflush(NULL);
    pid_t pid = fork();
    if (pid == 0) {
        close(pipefd[0]);
        dup2(pipefd[1], STDERR_FILENO);
        alarm(5);
        FILE *f = open_source(text);
        scanner_conf_t conf;
        scanner_init(&conf, f);
        ast_node_t *root = NULL;
        parse_result_t r = parse(&root, &conf);
        if (!r.success) {
            _exit(101);
        }
        symtable_stack_t stack;
        bool ok = semantic_analysis(root, &stack);
        fflush(stderr);
        _exit(ok ? 100 : 102);
    }
    close(pipefd[1]);
    char out[4096] = {0};
    size_t used = 0;
    ssize_t n;
    while ((n = read(pipefd[0], out + used, sizeof(out) - 1 - used)) > 0) {
        used += (size_t)n;
    }
    close(pipefd[0]);
    int status = 0;
    waitpid(pid, &status, 0);
    if (!WIFEXITED(status)) {
        return -2;
    }
    int code = WEXITSTATUS(status);
    if (code == 101) {
        return -1;
    }
    const char *marker = "Semantic analysis Error ";
    char *p = strstr(out, marker);
    if (p != NULL) {
        return atoi(p + strlen(marker));
    }
    return code == 100 ? 0 : -3;
}

#define ASSERT_SEMANTIC(expected, text)                                                            \
    do {                                                                                           \
        int got_ = analyze_source(text);                                                           \
        if (got_ != (expected)) {                                                                  \
            print_error("expected semantic result %d, got %d\n--- source ---\n%s\n", (expected),   \
                        got_, text);                                                               \
        }                                                                                          \
        assert_int_equal(got_, (expected));                                                        \
    } while (0)

#define ASSERT_SEMANTIC_OK(text) ASSERT_SEMANTIC(0, text)

// ---- valid programs ----

static void test_prog_minimal_main(void **state) {
    (void)state;
    ASSERT_SEMANTIC_OK(PROLOG MAIN_HDR "  ()\n");
}

static void test_prog_variables_and_inference(void **state) {
    (void)state;
    ASSERT_SEMANTIC_OK(PROLOG MAIN_HDR "  val a = 1\n"
                                       "  var b = 2.5\n"
                                       "  val c = \"s\"\n"
                                       "  var d: Int = a\n"
                                       "  d = d + 1\n"
                                       "  b = b * 2\n");
}

static void test_prog_int_widening_everywhere(void **state) {
    (void)state;
    ASSERT_SEMANTIC_OK(PROLOG "def half(x: Double): Double = x / 2\n" MAIN_HDR
                              "  val a: Double = 1\n"
                              "  var b = 1.5\n"
                              "  b = 2\n"
                              "  val c = a + 1\n"
                              "  half(3)\n");
}

static void test_prog_string_concatenation(void **state) {
    (void)state;
    ASSERT_SEMANTIC_OK(PROLOG MAIN_HDR "  val s = \"a\" + \"b\" + \"c\"\n  print(s)\n");
}

static void test_prog_function_calls_and_forward_reference(void **state) {
    (void)state;
    ASSERT_SEMANTIC_OK(PROLOG MAIN_HDR "  val r = add(1, 2)\n  print(r)\n"
                                       "def add(a: Int, b: Int): Int = a + b\n");
}

static void test_prog_recursion_with_explicit_type(void **state) {
    (void)state;
    ASSERT_SEMANTIC_OK(PROLOG "def fact(n: Int): Int =\n"
                              "  if n < 2 then\n"
                              "    return 1\n"
                              "  else\n"
                              "    return n * fact(n - 1)\n" MAIN_HDR "  print(fact(5))\n");
}

static void test_prog_mutual_recursion(void **state) {
    (void)state;
    ASSERT_SEMANTIC_OK(PROLOG
                       "def even(n: Int): Int =\n"
                       "  if n == 0 then\n    return 1\n  else\n    return odd(n - 1)\n"
                       "def odd(n: Int): Int =\n"
                       "  if n == 0 then\n    return 0\n  else\n    return even(n - 1)\n" MAIN_HDR
                       "  print(even(4))\n");
}

static void test_prog_while_and_if(void **state) {
    (void)state;
    ASSERT_SEMANTIC_OK(
        PROLOG MAIN_HDR
        "  var i = 0\n"
        "  while i < 3 do\n"
        "    if i == 1 then\n      print(\"one\")\n    else\n      print(\"other\")\n"
        "    i = i + 1\n");
}

static void test_prog_shadowing_in_nested_block(void **state) {
    (void)state;
    ASSERT_SEMANTIC_OK(PROLOG MAIN_HDR
                       "  val x = 1\n"
                       "  if x == 1 then\n    val x = \"inner\"\n    print(x)\n  else\n    ()\n");
}

static void test_prog_parameters_are_visible_in_body(void **state) {
    (void)state;
    ASSERT_SEMANTIC_OK(PROLOG "def f(a: Int, b: String): String =\n  val t = a + 1\n  b\n" MAIN_HDR
                              "  print(f(1, \"x\"))\n");
}

static void test_prog_builtin_functions(void **state) {
    (void)state;
    ASSERT_SEMANTIC_OK(PROLOG MAIN_HDR "  val a = readInt()\n"
                                       "  val b = readDouble()\n"
                                       "  val c = readLine()\n"
                                       "  val d = toStr(a)\n"
                                       "  val e = toInt(b)\n"
                                       "  val f = strLength(c)\n"
                                       "  val g = subStr(c, 0, 1)\n"
                                       "  val h = ord(c, 0)\n"
                                       "  val i = chr(65)\n"
                                       "  print(d)\n");
}

static void test_prog_main_with_other_function_defined_before(void **state) {
    (void)state;
    ASSERT_SEMANTIC_OK(PROLOG "def helper(): Int = 1\n" MAIN_HDR "  helper()\n");
}

static void test_prog_unit_function_result_can_be_ignored(void **state) {
    (void)state;
    ASSERT_SEMANTIC_OK(PROLOG "def f(): Unit =\n  print(1)\n" MAIN_HDR "  f()\n");
}

// ---- error 3: undefined ----

static void test_prog_error3_undefined_variable(void **state) {
    (void)state;
    ASSERT_SEMANTIC(3, PROLOG MAIN_HDR "  print(x)\n");
    ASSERT_SEMANTIC(3, PROLOG MAIN_HDR "  val a = b + 1\n");
}

static void test_prog_error3_use_before_definition(void **state) {
    (void)state;
    ASSERT_SEMANTIC(3, PROLOG MAIN_HDR "  val a = b\n  val b = 1\n");
}

static void test_prog_error3_variable_out_of_scope(void **state) {
    (void)state;
    ASSERT_SEMANTIC(3, PROLOG MAIN_HDR "  if 1 < 2 then\n    val inner = 1\n  else\n    ()\n"
                                       "  print(inner)\n");
}

static void test_prog_error3_undefined_function(void **state) {
    (void)state;
    ASSERT_SEMANTIC(3, PROLOG MAIN_HDR "  nothing(1)\n");
    ASSERT_SEMANTIC(3, PROLOG MAIN_HDR "  val a = undefinedFn()\n");
}

static void test_prog_error3_assignment_to_undefined(void **state) {
    (void)state;
    ASSERT_SEMANTIC(3, PROLOG MAIN_HDR "  x = 1\n");
}

static void test_prog_error3_missing_main(void **state) {
    (void)state;
    ASSERT_SEMANTIC(3, PROLOG "def f(): Int = 1\n");
}

static void test_prog_error3_parameter_of_other_function_not_visible(void **state) {
    (void)state;
    ASSERT_SEMANTIC(3, PROLOG "def f(a: Int): Int = a\ndef g(): Int = a\n" MAIN_HDR "  ()\n");
}

// ---- error 4: redefinition ----

static void test_prog_error4_function_redefinition(void **state) {
    (void)state;
    ASSERT_SEMANTIC(4, PROLOG "def f(): Int = 1\ndef f(): Int = 2\n" MAIN_HDR "  ()\n");
}

static void test_prog_error4_overloading_is_not_allowed(void **state) {
    (void)state;
    ASSERT_SEMANTIC(4,
                    PROLOG "def f(a: Int): Int = 1\ndef f(a: String): Int = 2\n" MAIN_HDR "  ()\n");
}

static void test_prog_error4_variable_redefinition_in_same_block(void **state) {
    (void)state;
    ASSERT_SEMANTIC(4, PROLOG MAIN_HDR "  val x = 1\n  val x = 2\n");
    ASSERT_SEMANTIC(4, PROLOG MAIN_HDR "  val x = 1\n  var x = \"a\"\n");
}

static void test_prog_error4_duplicate_main(void **state) {
    (void)state;
    ASSERT_SEMANTIC(4, PROLOG "@main def a(): Unit =\n  ()\n@main def b(): Unit =\n  ()\n");
}

static void test_prog_error4_user_function_named_like_builtin(void **state) {
    (void)state;
    ASSERT_SEMANTIC(4, PROLOG "def print(a: Int): Unit =\n  ()\n" MAIN_HDR "  ()\n");
}

static void test_prog_error4_duplicate_parameter_names(void **state) {
    (void)state;
    ASSERT_SEMANTIC(4, PROLOG "def f(a: Int, a: Int): Int = a\n" MAIN_HDR "  ()\n");
}

// ---- error 5: wrong arguments / main signature ----

static void test_prog_error5_too_few_arguments(void **state) {
    (void)state;
    ASSERT_SEMANTIC(5, PROLOG "def f(a: Int, b: Int): Int = a\n" MAIN_HDR "  f(1)\n");
}

static void test_prog_error5_too_many_arguments(void **state) {
    (void)state;
    ASSERT_SEMANTIC(5, PROLOG "def f(a: Int): Int = a\n" MAIN_HDR "  f(1, 2)\n");
}

static void test_prog_error5_wrong_argument_type(void **state) {
    (void)state;
    ASSERT_SEMANTIC(5, PROLOG "def f(a: Int): Int = a\n" MAIN_HDR "  f(\"x\")\n");
    ASSERT_SEMANTIC(5, PROLOG "def f(a: Int): Int = a\n" MAIN_HDR "  f(1.5)\n");
}

static void test_prog_error5_builtin_wrong_arguments(void **state) {
    (void)state;
    ASSERT_SEMANTIC(5, PROLOG MAIN_HDR "  val a = strLength(5)\n");
    ASSERT_SEMANTIC(5, PROLOG MAIN_HDR "  val a = readInt(1)\n");
    ASSERT_SEMANTIC(5, PROLOG MAIN_HDR "  val a = chr(\"a\")\n");
    ASSERT_SEMANTIC(5, PROLOG MAIN_HDR "  val a = subStr(\"abc\", 1)\n");
    ASSERT_SEMANTIC(5, PROLOG MAIN_HDR "  val a = toInt(1)\n");
}

static void test_prog_error5_main_with_parameter(void **state) {
    (void)state;
    ASSERT_SEMANTIC(5, PROLOG "@main def main(a: Int): Unit =\n  ()\n");
}

// ---- error 6: types ----

static void test_prog_error6_binary_operator_type_mismatch(void **state) {
    (void)state;
    ASSERT_SEMANTIC(6, PROLOG MAIN_HDR "  val a = 1 + \"x\"\n");
    ASSERT_SEMANTIC(6, PROLOG MAIN_HDR "  val a = \"x\" * 2\n");
    ASSERT_SEMANTIC(6, PROLOG MAIN_HDR "  val a = \"x\" - \"y\"\n");
}

static void test_prog_error6_initialization_type_mismatch(void **state) {
    (void)state;
    ASSERT_SEMANTIC(6, PROLOG MAIN_HDR "  val a: Int = \"s\"\n");
    ASSERT_SEMANTIC(6, PROLOG MAIN_HDR "  val a: Int = 1.5\n");
    ASSERT_SEMANTIC(6, PROLOG MAIN_HDR "  val a: String = 1\n");
}

static void test_prog_error6_assignment_type_mismatch(void **state) {
    (void)state;
    ASSERT_SEMANTIC(6, PROLOG MAIN_HDR "  var a = 1\n  a = \"s\"\n");
    ASSERT_SEMANTIC(6, PROLOG MAIN_HDR "  var a = 1\n  a = 1.5\n");
    ASSERT_SEMANTIC(6, PROLOG MAIN_HDR "  var a = \"s\"\n  a = 1\n");
}

static void test_prog_error6_return_type_mismatch(void **state) {
    (void)state;
    ASSERT_SEMANTIC(6, PROLOG "def f(): Int =\n  return \"s\"\n" MAIN_HDR "  ()\n");
    ASSERT_SEMANTIC(6, PROLOG "def f(): Int = \"s\"\n" MAIN_HDR "  ()\n");
    ASSERT_SEMANTIC(6, PROLOG "def f(): String =\n  val x = 1\n  x\n" MAIN_HDR "  ()\n");
}

static void test_prog_error6_main_return_type_not_unit(void **state) {
    (void)state;
    ASSERT_SEMANTIC(6, PROLOG "@main def main(): Int =\n  1\n");
}

static void test_prog_error6_condition_not_relational(void **state) {
    (void)state;
    ASSERT_SEMANTIC(6, PROLOG MAIN_HDR "  if 1 then\n    ()\n  else\n    ()\n");
    ASSERT_SEMANTIC(6, PROLOG MAIN_HDR "  while \"a\" do\n    ()\n");
}

static void test_prog_error6_comparing_incompatible_types(void **state) {
    (void)state;
    ASSERT_SEMANTIC(6, PROLOG MAIN_HDR "  if 1 < \"a\" then\n    ()\n  else\n    ()\n");
    ASSERT_SEMANTIC(6, PROLOG MAIN_HDR "  if \"a\" == 1 then\n    ()\n  else\n    ()\n");
}

static void test_prog_error6_cannot_infer_type_from_unit_call_chain(void **state) {
    (void)state;
    // function without explicit return type and with a return statement (spec 2.3: explicit type
    // required)
    ASSERT_SEMANTIC(6, PROLOG "def f() =\n  return 1\n" MAIN_HDR "  ()\n");
}

static void test_prog_error6_recursion_without_explicit_type(void **state) {
    (void)state;
    ASSERT_SEMANTIC(6, PROLOG "def f(n: Int) = f(n)\n" MAIN_HDR "  ()\n");
}

static void test_prog_error6_wrong_type_in_builtin_condition(void **state) {
    (void)state;
    ASSERT_SEMANTIC(6, PROLOG MAIN_HDR "  val a: String = readInt()\n");
}

// ---- error 7: immutability ----

static void test_prog_error7_assignment_to_val(void **state) {
    (void)state;
    ASSERT_SEMANTIC(7, PROLOG MAIN_HDR "  val x = 1\n  x = 2\n");
}

static void test_prog_error7_assignment_to_parameter(void **state) {
    (void)state;
    ASSERT_SEMANTIC(7, PROLOG "def f(a: Int): Int =\n  a = 2\n  a\n" MAIN_HDR "  ()\n");
}

static void test_prog_error7_assignment_to_val_in_nested_block(void **state) {
    (void)state;
    ASSERT_SEMANTIC(7, PROLOG MAIN_HDR "  val x = 1\n  while 1 < 2 do\n    x = 2\n");
}

// ---- other ----

static void test_prog_errors_inside_nested_blocks_are_detected(void **state) {
    (void)state;
    ASSERT_SEMANTIC(
        3, PROLOG MAIN_HDR
        "  while 1 < 2 do\n    if 1 < 2 then\n      print(undefinedVar)\n    else\n      ()\n");
}

static void test_prog_error_in_non_main_function_is_detected(void **state) {
    (void)state;
    ASSERT_SEMANTIC(3, PROLOG "def f(): Int = missing\n" MAIN_HDR "  ()\n");
}

static void test_prog_error_in_main_function_is_detected(void **state) {
    (void)state;
    ASSERT_SEMANTIC(3, PROLOG MAIN_HDR "  missing\n");
}

#define TF(fn) cmocka_unit_test_setup_teardown(fn, setup_fixture, teardown_fixture)
#define TP(fn) cmocka_unit_test_setup_teardown(fn, test_setup_timeout, test_teardown_timeout)

int main(void) {
    const struct CMUnitTest tests[] = {
        TP(test_semantic_init_null_stack_fails),
        TF(test_semantic_init_creates_global_scope),
        TP(test_count_fun_params),
        TF(test_create_conversion_node),
        TF(test_conversion_of_int_constant_is_done_in_place),
        TF(test_conversion_of_int_expression_wraps_node),
        TF(test_conversion_of_double_is_noop),
        TF(test_conversion_of_string_is_error),
        TF(test_conversion_null_arguments),
        TP(test_literal_nodes_carry_their_result_type),
        TF(test_check_expr_null_is_error),
        TF(test_check_expr_literals),
        TF(test_check_expr_invalid_node_type_is_error),
        TF(test_check_variable_expression),
        TF(test_check_undefined_variable_is_error),
        TF(test_operator_int_int),
        TF(test_operator_double_double),
        TF(test_operator_int_and_double_converts_int_side),
        TF(test_operator_string_concatenation),
        TF(test_operator_string_and_int_is_error),
        TF(test_operator_minus_on_strings_is_error),
        TF(test_operator_error_in_operand_propagates),
        TF(test_operator_unary_minus_keeps_type),
        TF(test_unary_minus_on_string_is_error),
        TF(test_call_undefined_function_is_error),
        TF(test_call_returns_function_return_type),
        TF(test_call_without_parameters),
        TF(test_call_with_too_few_arguments),
        TF(test_call_with_too_many_arguments),
        TF(test_call_with_wrong_argument_type),
        TF(test_call_double_argument_for_int_parameter_is_error),
        TF(test_call_int_constant_for_double_parameter_is_converted),
        TF(test_call_int_variable_for_double_parameter_stays_linked),
        TF(test_declaration_infers_type),
        TF(test_declaration_with_matching_explicit_type),
        TF(test_declaration_int_to_double_conversion),
        TF(test_declaration_type_mismatch_is_error),
        TF(test_declaration_double_to_int_is_error),
        TF(test_declaration_redefinition_in_same_scope_is_error),
        TF(test_declaration_shadowing_in_inner_scope_is_ok),
        TF(test_declaration_without_initializer_is_error),
        TF(test_declarations_get_unique_ids),
        TF(test_declaration_unique_id_is_copied_to_ast),
        TF(test_assignment_ok),
        TF(test_assignment_to_undefined_variable_is_error),
        TF(test_assignment_to_immutable_is_error),
        TF(test_assignment_type_mismatch_is_error),
        TF(test_assignment_int_to_double_variable_is_converted),
        TF(test_assignment_double_to_int_variable_is_error),
        TF(test_assignment_copies_unique_id_to_ast),
        TF(test_return_matching_type),
        TF(test_return_int_in_double_function_is_converted),
        TF(test_return_wrong_type_is_error),
        TF(test_bare_return_in_unit_function_is_ok),
        TF(test_return_value_in_unit_function_is_discarded_not_error),
        TF(test_check_block_null_is_unit),
        TF(test_check_block_empty),
        TF(test_check_block_scope_is_popped),
        TF(test_check_block_sees_outer_variables),
        TF(test_check_block_error_stops_and_restores_scope),
        TF(test_check_block_expression_statements),
        TF(test_check_block_variable_expression_statement),
        TF(test_if_checks_both_branches),
        TF(test_if_condition_must_be_relational),
        TF(test_while_condition_must_be_relational),
        TF(test_while_with_relational_condition_is_ok),
        TF(test_first_pass_null_arguments),
        TF(test_second_pass_null_arguments),
        TP(test_semantic_analysis_null_arguments),
        TP(test_prog_minimal_main),
        TP(test_prog_variables_and_inference),
        TP(test_prog_int_widening_everywhere),
        TP(test_prog_string_concatenation),
        TP(test_prog_function_calls_and_forward_reference),
        TP(test_prog_recursion_with_explicit_type),
        TP(test_prog_mutual_recursion),
        TP(test_prog_while_and_if),
        TP(test_prog_shadowing_in_nested_block),
        TP(test_prog_parameters_are_visible_in_body),
        TP(test_prog_builtin_functions),
        TP(test_prog_main_with_other_function_defined_before),
        TP(test_prog_unit_function_result_can_be_ignored),
        TP(test_prog_error3_undefined_variable),
        TP(test_prog_error3_use_before_definition),
        TP(test_prog_error3_variable_out_of_scope),
        TP(test_prog_error3_undefined_function),
        TP(test_prog_error3_assignment_to_undefined),
        TP(test_prog_error3_missing_main),
        TP(test_prog_error3_parameter_of_other_function_not_visible),
        TP(test_prog_error4_function_redefinition),
        TP(test_prog_error4_overloading_is_not_allowed),
        TP(test_prog_error4_variable_redefinition_in_same_block),
        TP(test_prog_error4_duplicate_main),
        TP(test_prog_error4_user_function_named_like_builtin),
        TP(test_prog_error4_duplicate_parameter_names),
        TP(test_prog_error5_too_few_arguments),
        TP(test_prog_error5_too_many_arguments),
        TP(test_prog_error5_wrong_argument_type),
        TP(test_prog_error5_builtin_wrong_arguments),
        TP(test_prog_error5_main_with_parameter),
        TP(test_prog_error6_binary_operator_type_mismatch),
        TP(test_prog_error6_initialization_type_mismatch),
        TP(test_prog_error6_assignment_type_mismatch),
        TP(test_prog_error6_return_type_mismatch),
        TP(test_prog_error6_main_return_type_not_unit),
        TP(test_prog_error6_condition_not_relational),
        TP(test_prog_error6_comparing_incompatible_types),
        TP(test_prog_error6_cannot_infer_type_from_unit_call_chain),
        TP(test_prog_error6_recursion_without_explicit_type),
        TP(test_prog_error6_wrong_type_in_builtin_condition),
        TP(test_prog_error7_assignment_to_val),
        TP(test_prog_error7_assignment_to_parameter),
        TP(test_prog_error7_assignment_to_val_in_nested_block),
        TP(test_prog_errors_inside_nested_blocks_are_detected),
        TP(test_prog_error_in_non_main_function_is_detected),
        TP(test_prog_error_in_main_function_is_detected),
    };
    return cmocka_run_group_tests_name("semantic", tests, NULL, NULL);
}
