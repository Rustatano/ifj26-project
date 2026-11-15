/*
-------------------------------------------------------
Name of project: Translator IFJ26
File: test_parser.c

Description: Unit tests (cmocka) of the parser: program structure,
             functions, statements, blocks and expressions
             (recursive descent + precedence analysis).
             Lexical errors terminate the process (exit(1) in the
             scanner), so they are checked in a forked child.
-------------------------------------------------------
*/

#define _GNU_SOURCE
#include "ast.h"
#include "parser.h"
#include "test_helpers.h"
#include <fcntl.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

#define T(fn) cmocka_unit_test_setup_teardown(fn, test_setup_timeout, test_teardown_timeout)

#define PROLOG "import ifj26.*\n"
#define MAIN_HDR "@main def main(): Unit =\n"

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

typedef struct {
    parse_result_t result;
    ast_node_t *root;
} parsed_t;

static parsed_t parse_text(const char *text) {
    parsed_t p;
    FILE *f = open_source(text);
    scanner_conf_t conf;
    scanner_init(&conf, f);
    p.root = NULL;
    p.result = parse(&p.root, &conf);
    fclose(f);
    return p;
}

static void parsed_free(parsed_t *p) { ast_dispose(&p->root); }

// Parses text in a child process. Returns:
//   0 = parsed OK, 2 = syntax error, 3 = internal error,
//   1 = lexical error (scanner called exit(1)), -1000-sig = crashed, 99 = other
static int parse_status_in_child(const char *text) {
    fflush(NULL);
    pid_t pid = fork();
    if (pid == 0) {
        int devnull = open("/dev/null", O_WRONLY);
        dup2(devnull, STDERR_FILENO);
        alarm(5);
        parsed_t p = parse_text(text);
        _exit(p.result.success ? 0 : (int)p.result.error_code);
    }
    int status = 0;
    waitpid(pid, &status, 0);
    if (WIFEXITED(status)) {
        return WEXITSTATUS(status);
    }
    if (WIFSIGNALED(status)) {
        return -1000 - WTERMSIG(status);
    }
    return 99;
}

#define ASSERT_PARSES(text)                                                                        \
    do {                                                                                           \
        parsed_t p_ = parse_text(text);                                                            \
        if (!p_.result.success) {                                                                  \
            print_error("expected success, got error %d: %s\n--- source ---\n%s\n",                \
                        p_.result.error_code, p_.result.error_msg ? p_.result.error_msg : "-",     \
                        text);                                                                     \
        }                                                                                          \
        assert_true(p_.result.success);                                                            \
        assert_non_null(p_.root);                                                                  \
        parsed_free(&p_);                                                                          \
    } while (0)

#define ASSERT_SYNTAX_ERROR(text)                                                                  \
    do {                                                                                           \
        parsed_t p_ = parse_text(text);                                                            \
        if (p_.result.success) {                                                                   \
            print_error("expected syntax error, but parsing succeeded\n--- source ---\n%s\n",      \
                        text);                                                                     \
        }                                                                                          \
        assert_false(p_.result.success);                                                           \
        assert_int_equal(p_.result.error_code, PARSE_ERR_SYNTAX);                                  \
        assert_null(p_.root);                                                                      \
        parsed_free(&p_);                                                                          \
    } while (0)

#define ASSERT_LEX_ERROR(text) assert_int_equal(parse_status_in_child(text), 1)

// Returns the first function node (MAIN or FUN_DEFINITION) after the prolog.
static ast_node_t *first_function(ast_node_t *root) {
    assert_non_null(root);
    assert_int_equal(root->type, AST_NODE_BLOCK);
    ast_node_t *n = root->block.first_statement;
    assert_non_null(n);
    assert_int_equal(n->type, AST_NODE_STATEMENT_IMPORT);
    n = n->next;
    assert_non_null(n);
    return n;
}

// Returns first statement of the first function body.
static ast_node_t *first_statement(ast_node_t *root) {
    ast_node_t *fn = first_function(root);
    assert_non_null(fn->fun_definition.body_block);
    ast_node_t *s = fn->fun_definition.body_block->block.first_statement;
    assert_non_null(s);
    return s;
}

// Parses "val a = <expr>" in main and returns the init expression node.
static ast_node_t *parse_expression_text(const char *expr, parsed_t *out) {
    char src[1024];
    snprintf(src, sizeof(src), PROLOG MAIN_HDR "  val a = %s\n", expr);
    *out = parse_text(src);
    if (!out->result.success) {
        print_error("expression '%s' failed: %s\n", expr,
                    out->result.error_msg ? out->result.error_msg : "-");
    }
    assert_true(out->result.success);
    ast_node_t *decl = first_statement(out->root);
    assert_int_equal(decl->type, AST_NODE_VAR_DECLARATION);
    return decl->var_declaration.init_expression;
}

static void assert_int_const(ast_node_t *n, int value) {
    assert_non_null(n);
    assert_int_equal(n->type, AST_NODE_CONST_INT);
    assert_int_equal(n->int_value, value);
}

static void assert_op(ast_node_t *n, ast_expression_operator_t op) {
    assert_non_null(n);
    assert_int_equal(n->type, AST_NODE_EXPRESSION_OPERATOR);
    assert_int_equal(n->expression_operator.op, op);
}

// ---------------------------------------------------------------------------
// Program structure (prolog, @main, top level)
// ---------------------------------------------------------------------------

static void test_minimal_program(void **state) {
    (void)state;
    ASSERT_PARSES(PROLOG MAIN_HDR "  ()\n");
}

static void test_program_root_is_block_with_import_first(void **state) {
    (void)state;
    parsed_t p = parse_text(PROLOG MAIN_HDR "  ()\n");
    assert_true(p.result.success);
    assert_int_equal(p.result.error_code, PARSE_SUCCESS);
    ast_node_t *imp = p.root->block.first_statement;
    assert_int_equal(imp->type, AST_NODE_STATEMENT_IMPORT);
    assert_string_equal(imp->import_statement.module_name, "ifj26.*");
    assert_int_equal(imp->next->type, AST_NODE_MAIN);
    parsed_free(&p);
}

static void test_prolog_may_be_preceded_by_comments_and_blank_lines(void **state) {
    (void)state;
    ASSERT_PARSES("// comment\n\n/* block */\n" PROLOG MAIN_HDR "  ()\n");
}

static void test_prolog_allows_spaces_between_tokens(void **state) {
    (void)state;
    ASSERT_PARSES("import   ifj26 . *\n" MAIN_HDR "  ()\n");
}

static void test_missing_prolog_is_syntax_error(void **state) {
    (void)state;
    ASSERT_SYNTAX_ERROR(MAIN_HDR "  ()\n");
}

static void test_wrong_prolog_is_syntax_error(void **state) {
    (void)state;
    ASSERT_SYNTAX_ERROR("import ifj27.*\n" MAIN_HDR "  ()\n");
    ASSERT_SYNTAX_ERROR("import ifj26\n" MAIN_HDR "  ()\n");
    ASSERT_SYNTAX_ERROR("import ifj26.\n" MAIN_HDR "  ()\n");
    ASSERT_SYNTAX_ERROR("import *\n" MAIN_HDR "  ()\n");
    ASSERT_SYNTAX_ERROR("import ifj26.* 1\n" MAIN_HDR "  ()\n");
}

static void test_duplicate_prolog_is_syntax_error(void **state) {
    (void)state;
    ASSERT_SYNTAX_ERROR(PROLOG PROLOG MAIN_HDR "  ()\n");
}

static void test_empty_input_is_syntax_error(void **state) {
    (void)state;
    ASSERT_SYNTAX_ERROR("");
    ASSERT_SYNTAX_ERROR("\n\n");
}

static void test_only_prolog_without_function_is_not_success(void **state) {
    (void)state;
    // there is no @main function at all
    parsed_t p = parse_text(PROLOG);
    assert_false(p.result.success);
    parsed_free(&p);
}

static void test_main_annotation_on_separate_line(void **state) {
    (void)state;
    parsed_t p = parse_text(PROLOG "@main\ndef run(): Unit =\n  ()\n");
    assert_true(p.result.success);
    assert_int_equal(first_function(p.root)->type, AST_NODE_MAIN);
    assert_string_equal(first_function(p.root)->fun_definition.name, "run");
    parsed_free(&p);
}

static void test_main_annotation_on_same_line(void **state) {
    (void)state;
    parsed_t p = parse_text(PROLOG "@main def run(): Unit =\n  ()\n");
    assert_true(p.result.success);
    assert_int_equal(first_function(p.root)->type, AST_NODE_MAIN);
    parsed_free(&p);
}

static void test_main_can_have_any_identifier(void **state) {
    (void)state;
    ASSERT_PARSES(PROLOG "@main def whatever_42(): Unit =\n  ()\n");
}

static void test_functions_before_and_after_main(void **state) {
    (void)state;
    parsed_t p = parse_text(PROLOG "def a(): Int = 1\n" MAIN_HDR "  ()\n"
                                   "def b(): Int = 2\n");
    assert_true(p.result.success);
    ast_node_t *n = first_function(p.root);
    assert_int_equal(n->type, AST_NODE_FUN_DEFINITION);
    assert_int_equal(n->next->type, AST_NODE_MAIN);
    assert_int_equal(n->next->next->type, AST_NODE_FUN_DEFINITION);
    assert_null(n->next->next->next);
    parsed_free(&p);
}

static void test_missing_main_is_not_a_syntax_error(void **state) {
    (void)state;
    // Spec section 6: missing @main function is a SEMANTIC error (code 3),
    // so the parser must accept the program and leave the decision to semantic analysis.
    parsed_t p = parse_text(PROLOG "def f(): Int = 1\n");
    if (!p.result.success) {
        print_error("parser rejected a program without @main with code %d (spec: 3)\n",
                    p.result.error_code);
    }
    assert_true(p.result.success);
    parsed_free(&p);
}

static void test_duplicate_main_is_not_a_syntax_error(void **state) {
    (void)state;
    // Spec section 2: more @main functions = semantic error 4.
    parsed_t p = parse_text(PROLOG "@main def a(): Unit =\n  ()\n@main def b(): Unit =\n  ()\n");
    if (!p.result.success) {
        print_error("parser rejected duplicate @main with code %d (spec: 4 = semantic)\n",
                    p.result.error_code);
    }
    assert_true(p.result.success);
    parsed_free(&p);
}

static void test_annotation_without_def_is_syntax_error(void **state) {
    (void)state;
    ASSERT_SYNTAX_ERROR(PROLOG "@main\nval x = 1\n");
    ASSERT_SYNTAX_ERROR(PROLOG "@main\n");
}

static void test_statement_at_top_level_is_syntax_error(void **state) {
    (void)state;
    ASSERT_SYNTAX_ERROR(PROLOG "val x = 1\n" MAIN_HDR "  ()\n");
    ASSERT_SYNTAX_ERROR(PROLOG MAIN_HDR "  ()\nprint(1)\n");
}

// ---------------------------------------------------------------------------
// Function definitions
// ---------------------------------------------------------------------------

static void test_function_without_parameters(void **state) {
    (void)state;
    parsed_t p = parse_text(PROLOG "def f(): Int =\n  1\n" MAIN_HDR "  ()\n");
    assert_true(p.result.success);
    ast_node_t *f = first_function(p.root);
    assert_string_equal(f->fun_definition.name, "f");
    assert_null(f->fun_definition.params);
    assert_int_equal(f->fun_definition.return_type, SYM_DATATYPE_INT);
    parsed_free(&p);
}

static void test_function_parameters_are_parsed_in_order(void **state) {
    (void)state;
    parsed_t p = parse_text(
        PROLOG "def f(a: Int, b: Double, c: String, d: Unit): Int = 1\n" MAIN_HDR "  ()\n");
    assert_true(p.result.success);
    symtable_fun_param_t *prm = first_function(p.root)->fun_definition.params;
    assert_non_null(prm);
    assert_string_equal(prm->name, "a");
    assert_int_equal(prm->type, SYM_DATATYPE_INT);
    prm = prm->next;
    assert_string_equal(prm->name, "b");
    assert_int_equal(prm->type, SYM_DATATYPE_DOUBLE);
    prm = prm->next;
    assert_string_equal(prm->name, "c");
    assert_int_equal(prm->type, SYM_DATATYPE_STRING);
    prm = prm->next;
    assert_string_equal(prm->name, "d");
    assert_int_equal(prm->type, SYM_DATATYPE_UNIT);
    assert_null(prm->next);
    parsed_free(&p);
}

static void test_return_type_may_be_omitted(void **state) {
    (void)state;
    parsed_t p = parse_text(PROLOG "def f() = 1\n" MAIN_HDR "  ()\n");
    assert_true(p.result.success);
    assert_int_equal(first_function(p.root)->fun_definition.return_type, SYM_DATATYPE_UNKNOWN);
    parsed_free(&p);
}

static void test_all_return_types(void **state) {
    (void)state;
    const char *types[] = {"Int", "Double", "String", "Unit"};
    symtable_datatype_t expected[] = {SYM_DATATYPE_INT, SYM_DATATYPE_DOUBLE, SYM_DATATYPE_STRING,
                                      SYM_DATATYPE_UNIT};
    for (int i = 0; i < 4; i++) {
        char src[256];
        snprintf(src, sizeof(src), PROLOG "def f(): %s = ()\n" MAIN_HDR "  ()\n", types[i]);
        parsed_t p = parse_text(src);
        assert_true(p.result.success);
        assert_int_equal(first_function(p.root)->fun_definition.return_type, expected[i]);
        parsed_free(&p);
    }
}

static void test_one_line_function_body_is_wrapped_in_block(void **state) {
    (void)state;
    parsed_t p = parse_text(PROLOG "def f(a: Int): Int = a + 1\n" MAIN_HDR "  ()\n");
    assert_true(p.result.success);
    ast_node_t *body = first_function(p.root)->fun_definition.body_block;
    assert_int_equal(body->type, AST_NODE_BLOCK);
    assert_op(body->block.first_statement, AST_EXPRESSION_OPERATOR_ADD);
    parsed_free(&p);
}

static void test_multiline_function_body(void **state) {
    (void)state;
    parsed_t p = parse_text(PROLOG "def f(): Int =\n  val x = 1\n  x\n" MAIN_HDR "  ()\n");
    assert_true(p.result.success);
    ast_node_t *s1 = first_function(p.root)->fun_definition.body_block->block.first_statement;
    assert_int_equal(s1->type, AST_NODE_VAR_DECLARATION);
    assert_int_equal(s1->next->type, AST_NODE_EXPRESSION_VAR);
    parsed_free(&p);
}

static void test_function_header_errors(void **state) {
    (void)state;
    ASSERT_SYNTAX_ERROR(PROLOG "def (): Int = 1\n" MAIN_HDR "  ()\n");    // no name
    ASSERT_SYNTAX_ERROR(PROLOG "def f: Int = 1\n" MAIN_HDR "  ()\n");     // no parens
    ASSERT_SYNTAX_ERROR(PROLOG "def f(: Int = 1\n" MAIN_HDR "  ()\n");    // unclosed
    ASSERT_SYNTAX_ERROR(PROLOG "def f() Int = 1\n" MAIN_HDR "  ()\n");    // missing ':'
    ASSERT_SYNTAX_ERROR(PROLOG "def f(): = 1\n" MAIN_HDR "  ()\n");       // missing type
    ASSERT_SYNTAX_ERROR(PROLOG "def f(): Int 1\n" MAIN_HDR "  ()\n");     // missing '='
    ASSERT_SYNTAX_ERROR(PROLOG "def f(): Foo = 1\n" MAIN_HDR "  ()\n");   // unknown type
    ASSERT_SYNTAX_ERROR(PROLOG "def f(): Int =\n" MAIN_HDR "  ()\n");     // missing body
    ASSERT_SYNTAX_ERROR(PROLOG "def def(): Int = 1\n" MAIN_HDR "  ()\n"); // keyword as name
}

static void test_function_parameter_errors(void **state) {
    (void)state;
    ASSERT_SYNTAX_ERROR(PROLOG "def f(a): Int = 1\n" MAIN_HDR "  ()\n");             // no type
    ASSERT_SYNTAX_ERROR(PROLOG "def f(a Int): Int = 1\n" MAIN_HDR "  ()\n");         // no colon
    ASSERT_SYNTAX_ERROR(PROLOG "def f(a: Int b: Int): Int = 1\n" MAIN_HDR "  ()\n"); // no comma
    ASSERT_SYNTAX_ERROR(PROLOG "def f(a: Foo): Int = 1\n" MAIN_HDR "  ()\n");        // bad type
    ASSERT_SYNTAX_ERROR(PROLOG "def f(1: Int): Int = 1\n" MAIN_HDR "  ()\n");        // not an id
    ASSERT_SYNTAX_ERROR(PROLOG "def f(a: Int,, b: Int): Int = 1\n" MAIN_HDR "  ()\n");
}

static void test_trailing_comma_in_parameters_requires_newline_form(void **state) {
    (void)state;
    // spec 1.2: trailing comma is allowed only before ')' on a following line
    ASSERT_PARSES(PROLOG "def f(a: Int,\n      b: Int,\n  ): Int = 1\n" MAIN_HDR "  ()\n");
}

static void test_trailing_comma_on_same_line_is_error(void **state) {
    (void)state;
    ASSERT_SYNTAX_ERROR(PROLOG "def f(a: Int,): Int = 1\n" MAIN_HDR "  ()\n");
}

static void test_one_line_function_with_trailing_tokens_is_error(void **state) {
    (void)state;
    ASSERT_SYNTAX_ERROR(PROLOG "def f(): Int = 1 2\n" MAIN_HDR "  ()\n");
}

// ---------------------------------------------------------------------------
// Statements
// ---------------------------------------------------------------------------

static void test_val_declaration(void **state) {
    (void)state;
    parsed_t p = parse_text(PROLOG MAIN_HDR "  val x = 1\n");
    assert_true(p.result.success);
    ast_node_t *d = first_statement(p.root);
    assert_int_equal(d->type, AST_NODE_VAR_DECLARATION);
    assert_string_equal(d->var_declaration.name, "x");
    assert_false(d->var_declaration.is_mutable);
    assert_int_equal(d->var_declaration.var_type, SYM_DATATYPE_UNKNOWN);
    assert_int_const(d->var_declaration.init_expression, 1);
    parsed_free(&p);
}

static void test_var_declaration_is_mutable(void **state) {
    (void)state;
    parsed_t p = parse_text(PROLOG MAIN_HDR "  var x = 1\n");
    assert_true(p.result.success);
    assert_true(first_statement(p.root)->var_declaration.is_mutable);
    parsed_free(&p);
}

static void test_declaration_with_explicit_type(void **state) {
    (void)state;
    parsed_t p = parse_text(PROLOG MAIN_HDR "  val x: Double = 1\n");
    assert_true(p.result.success);
    assert_int_equal(first_statement(p.root)->var_declaration.var_type, SYM_DATATYPE_DOUBLE);
    parsed_free(&p);
}

static void test_declaration_errors(void **state) {
    (void)state;
    ASSERT_SYNTAX_ERROR(PROLOG MAIN_HDR "  val x\n"); // initialization is mandatory
    ASSERT_SYNTAX_ERROR(PROLOG MAIN_HDR "  val x: Int\n");
    ASSERT_SYNTAX_ERROR(PROLOG MAIN_HDR "  val = 1\n");
    ASSERT_SYNTAX_ERROR(PROLOG MAIN_HDR "  val 1 = 1\n");
    ASSERT_SYNTAX_ERROR(PROLOG MAIN_HDR "  val x: = 1\n");
    ASSERT_SYNTAX_ERROR(PROLOG MAIN_HDR "  val x: Foo = 1\n");
    ASSERT_SYNTAX_ERROR(PROLOG MAIN_HDR "  val x = \n");
    ASSERT_SYNTAX_ERROR(PROLOG MAIN_HDR "  var x == 1\n");
}

static void test_assignment(void **state) {
    (void)state;
    parsed_t p = parse_text(PROLOG MAIN_HDR "  var x = 1\n  x = x + 2\n");
    assert_true(p.result.success);
    ast_node_t *a = first_statement(p.root)->next;
    assert_int_equal(a->type, AST_NODE_VALUE_ASSIGNMENT);
    assert_string_equal(a->assign_statement.target_var_name, "x");
    assert_op(a->assign_statement.expression, AST_EXPRESSION_OPERATOR_ADD);
    parsed_free(&p);
}

static void test_assignment_errors(void **state) {
    (void)state;
    ASSERT_SYNTAX_ERROR(PROLOG MAIN_HDR "  var x = 1\n  x =\n");
    ASSERT_SYNTAX_ERROR(PROLOG MAIN_HDR "  var x = 1\n  = 2\n");
    ASSERT_SYNTAX_ERROR(PROLOG MAIN_HDR "  var x = 1\n  1 = x\n");
}

static void test_expression_statement_function_call(void **state) {
    (void)state;
    parsed_t p = parse_text(PROLOG MAIN_HDR "  print(\"hi\")\n");
    assert_true(p.result.success);
    ast_node_t *c = first_statement(p.root);
    assert_int_equal(c->type, AST_NODE_EXPRESSION_FUN_CALL);
    assert_string_equal(c->expression_fun_call.fun_name, "print");
    assert_int_equal(c->expression_fun_call.params_count, 1);
    assert_int_equal(c->expression_fun_call.params->type, AST_NODE_CONST_STRING);
    assert_string_equal(c->expression_fun_call.params->str_value, "hi");
    parsed_free(&p);
}

static void test_statements_keep_source_order(void **state) {
    (void)state;
    parsed_t p = parse_text(PROLOG MAIN_HDR "  val a = 1\n  var b = 2\n  b = 3\n  print(b)\n");
    assert_true(p.result.success);
    ast_node_t *s = first_statement(p.root);
    assert_int_equal(s->type, AST_NODE_VAR_DECLARATION);
    s = s->next;
    assert_int_equal(s->type, AST_NODE_VAR_DECLARATION);
    s = s->next;
    assert_int_equal(s->type, AST_NODE_VALUE_ASSIGNMENT);
    s = s->next;
    assert_int_equal(s->type, AST_NODE_EXPRESSION_FUN_CALL);
    assert_null(s->next);
    parsed_free(&p);
}

static void test_semicolon_separated_statements(void **state) {
    (void)state;
    parsed_t p = parse_text(PROLOG MAIN_HDR "  val a = 1; val b = 2; print(a)\n");
    assert_true(p.result.success);
    ast_node_t *s = first_statement(p.root);
    assert_string_equal(s->var_declaration.name, "a");
    assert_string_equal(s->next->var_declaration.name, "b");
    assert_int_equal(s->next->next->type, AST_NODE_EXPRESSION_FUN_CALL);
    parsed_free(&p);
}

static void test_two_statements_on_one_line_without_separator_is_error(void **state) {
    (void)state;
    ASSERT_SYNTAX_ERROR(PROLOG MAIN_HDR "  val a = 1 val b = 2\n");
}

static void test_return_with_value(void **state) {
    (void)state;
    parsed_t p = parse_text(PROLOG "def f(): Int =\n  return 5\n" MAIN_HDR "  ()\n");
    assert_true(p.result.success);
    ast_node_t *r = first_statement(p.root);
    assert_int_equal(r->type, AST_NODE_STATEMENT_RETURN);
    assert_int_const(r->return_statement.expression, 5);
    parsed_free(&p);
}

static void test_return_without_value(void **state) {
    (void)state;
    parsed_t p = parse_text(PROLOG "def f(): Unit =\n  return\n" MAIN_HDR "  ()\n");
    assert_true(p.result.success);
    ast_node_t *r = first_statement(p.root);
    assert_int_equal(r->type, AST_NODE_STATEMENT_RETURN);
    assert_null(r->return_statement.expression);
    parsed_free(&p);
}

static void test_return_with_call(void **state) {
    (void)state;
    ASSERT_PARSES(PROLOG "def f(): Int =\n  return g(1)\ndef g(a: Int): Int = a\n" MAIN_HDR
                         "  ()\n");
}

static void test_if_else_statement(void **state) {
    (void)state;
    parsed_t p =
        parse_text(PROLOG MAIN_HDR "  if 1 < 2 then\n    print(1)\n  else\n    print(2)\n");
    assert_true(p.result.success);
    ast_node_t *i = first_statement(p.root);
    assert_int_equal(i->type, AST_NODE_STATEMENT_IF);
    assert_op(i->if_statement.condition, AST_EXPRESSION_OPERATOR_SMALLER);
    assert_int_equal(i->if_statement.if_block->type, AST_NODE_BLOCK);
    assert_int_equal(i->if_statement.else_block->type, AST_NODE_BLOCK);
    assert_int_equal(i->if_statement.if_block->block.first_statement->type,
                     AST_NODE_EXPRESSION_FUN_CALL);
    parsed_free(&p);
}

static void test_if_without_else_is_syntax_error_in_base_assignment(void **state) {
    (void)state;
    ASSERT_SYNTAX_ERROR(PROLOG MAIN_HDR "  if 1 < 2 then\n    print(1)\n");
    ASSERT_SYNTAX_ERROR(PROLOG MAIN_HDR "  if 1 < 2 then\n    print(1)\n  print(2)\n");
}

static void test_if_errors(void **state) {
    (void)state;
    ASSERT_SYNTAX_ERROR(PROLOG MAIN_HDR
                        "  if 1 < 2\n    print(1)\n  else\n    print(2)\n"); // no then
    ASSERT_SYNTAX_ERROR(PROLOG MAIN_HDR
                        "  if then\n    print(1)\n  else\n    print(2)\n");         // no cond
    ASSERT_SYNTAX_ERROR(PROLOG MAIN_HDR "  if 1 < 2 then\n  else\n    print(2)\n"); // empty block
    ASSERT_SYNTAX_ERROR(PROLOG MAIN_HDR "  if 1 < 2 then\n    print(1)\n  else\n"); // empty else
    ASSERT_SYNTAX_ERROR(
        PROLOG MAIN_HDR
        "  if 1 < 2 then print(1)\n  else\n    print(2)\n"); // 1-line needs CONTROL ext.
}

static void test_while_statement(void **state) {
    (void)state;
    parsed_t p = parse_text(PROLOG MAIN_HDR "  var i = 0\n  while i < 3 do\n    i = i + 1\n");
    assert_true(p.result.success);
    ast_node_t *w = first_statement(p.root)->next;
    assert_int_equal(w->type, AST_NODE_STATEMENT_WHILE);
    assert_op(w->while_statement.condition, AST_EXPRESSION_OPERATOR_SMALLER);
    assert_int_equal(w->while_statement.body_block->block.first_statement->type,
                     AST_NODE_VALUE_ASSIGNMENT);
    parsed_free(&p);
}

static void test_while_errors(void **state) {
    (void)state;
    ASSERT_SYNTAX_ERROR(PROLOG MAIN_HDR "  while 1 < 2\n    print(1)\n");  // no do
    ASSERT_SYNTAX_ERROR(PROLOG MAIN_HDR "  while do\n    print(1)\n");     // no cond
    ASSERT_SYNTAX_ERROR(PROLOG MAIN_HDR "  while 1 < 2 do\n");             // no body
    ASSERT_SYNTAX_ERROR(PROLOG MAIN_HDR "  while 1 < 2 do\n  print(1)\n"); // body not indented
}

static void test_nested_blocks(void **state) {
    (void)state;
    ASSERT_PARSES(PROLOG MAIN_HDR "  var i = 0\n"
                                  "  while i < 3 do\n"
                                  "    if i == 1 then\n"
                                  "      print(1)\n"
                                  "    else\n"
                                  "      while i < 2 do\n"
                                  "        i = i + 1\n"
                                  "    i = i + 1\n"
                                  "  print(\"end\")\n");
}

static void test_statement_after_dedent_belongs_to_outer_block(void **state) {
    (void)state;
    parsed_t p = parse_text(PROLOG MAIN_HDR "  while 1 < 2 do\n"
                                            "    print(1)\n"
                                            "  print(2)\n");
    assert_true(p.result.success);
    ast_node_t *w = first_statement(p.root);
    assert_int_equal(w->type, AST_NODE_STATEMENT_WHILE);
    assert_null(w->while_statement.body_block->block.first_statement->next);
    assert_non_null(w->next); // print(2) is a sibling of while
    assert_int_equal(w->next->type, AST_NODE_EXPRESSION_FUN_CALL);
    parsed_free(&p);
}

static void test_blank_lines_and_comments_inside_blocks(void **state) {
    (void)state;
    ASSERT_PARSES(PROLOG MAIN_HDR "\n"
                                  "  // comment\n"
                                  "  val a = 1 // trailing\n"
                                  "\n"
                                  "  /* block */\n"
                                  "  print(a)\n");
}

static void test_unit_literal_as_block_body(void **state) {
    (void)state;
    parsed_t p = parse_text(PROLOG MAIN_HDR "  if 1 < 2 then\n    ()\n  else\n    ()\n");
    assert_true(p.result.success);
    ast_node_t *i = first_statement(p.root);
    assert_int_equal(i->if_statement.if_block->block.first_statement->type, AST_NODE_CONST_UNIT);
    parsed_free(&p);
}

static void test_empty_function_block_is_error(void **state) {
    (void)state;
    ASSERT_SYNTAX_ERROR(PROLOG "@main def main(): Unit =\n");
    ASSERT_SYNTAX_ERROR(PROLOG "@main def main(): Unit =\n\n");
}

static void test_missing_indentation_is_syntax_error(void **state) {
    (void)state;
    ASSERT_SYNTAX_ERROR(PROLOG "@main def main(): Unit =\nprint(1)\n");
}

static void test_keyword_at_statement_start_is_error(void **state) {
    (void)state;
    ASSERT_SYNTAX_ERROR(PROLOG MAIN_HDR "  else\n    print(1)\n");
    ASSERT_SYNTAX_ERROR(PROLOG MAIN_HDR "  then\n");
    ASSERT_SYNTAX_ERROR(PROLOG MAIN_HDR "  do\n");
    ASSERT_SYNTAX_ERROR(PROLOG MAIN_HDR "  def f(): Int = 1\n");
    ASSERT_SYNTAX_ERROR(PROLOG MAIN_HDR "  Int\n");
}

static void test_main_body_ends_at_eof_without_newline(void **state) {
    (void)state;
    ASSERT_PARSES(PROLOG MAIN_HDR "  print(1)");
}

// ---------------------------------------------------------------------------
// Expressions (precedence analysis)
// ---------------------------------------------------------------------------

static void test_expr_literals(void **state) {
    (void)state;
    parsed_t p;
    ast_node_t *e = parse_expression_text("42", &p);
    assert_int_const(e, 42);
    parsed_free(&p);

    e = parse_expression_text("2.5", &p);
    assert_int_equal(e->type, AST_NODE_CONST_DOUBLE);
    assert_true(e->double_value > 2.49 && e->double_value < 2.51);
    parsed_free(&p);

    e = parse_expression_text("\"txt\"", &p);
    assert_int_equal(e->type, AST_NODE_CONST_STRING);
    assert_string_equal(e->str_value, "txt");
    parsed_free(&p);

    e = parse_expression_text("()", &p);
    assert_int_equal(e->type, AST_NODE_CONST_UNIT);
    parsed_free(&p);
}

static void test_expr_variable(void **state) {
    (void)state;
    parsed_t p;
    ast_node_t *e = parse_expression_text("abc", &p);
    assert_int_equal(e->type, AST_NODE_EXPRESSION_VAR);
    assert_string_equal(e->expression_var.name, "abc");
    parsed_free(&p);
}

static void test_expr_each_binary_operator_maps_correctly(void **state) {
    (void)state;
    struct {
        const char *expr;
        ast_expression_operator_t op;
    } cases[] = {
        {"1 + 2", AST_EXPRESSION_OPERATOR_ADD},
        {"1 - 2", AST_EXPRESSION_OPERATOR_SUB},
        {"1 * 2", AST_EXPRESSION_OPERATOR_MUL},
        {"1 / 2", AST_EXPRESSION_OPERATOR_DIV},
        {"1 == 2", AST_EXPRESSION_OPERATOR_EQUAL},
        {"1 != 2", AST_EXPRESSION_OPERATOR_NOT_EQUAL},
        {"1 < 2", AST_EXPRESSION_OPERATOR_SMALLER},
        {"1 <= 2", AST_EXPRESSION_OPERATOR_SMALLER_EQUAL},
        {"1 > 2", AST_EXPRESSION_OPERATOR_BIGGER},
        {"1 >= 2", AST_EXPRESSION_OPERATOR_BIGGER_EQUAL},
    };
    for (size_t i = 0; i < sizeof(cases) / sizeof(cases[0]); i++) {
        parsed_t p;
        ast_node_t *e = parse_expression_text(cases[i].expr, &p);
        if (e->type != AST_NODE_EXPRESSION_OPERATOR || e->expression_operator.op != cases[i].op) {
            print_error("'%s': got operator %d, expected %d\n", cases[i].expr,
                        e->expression_operator.op, cases[i].op);
        }
        assert_op(e, cases[i].op);
        assert_int_const(e->expression_operator.left_side, 1);
        assert_int_const(e->expression_operator.right_side, 2);
        parsed_free(&p);
    }
}

static void test_expr_mul_binds_tighter_than_add(void **state) {
    (void)state;
    parsed_t p;
    ast_node_t *e = parse_expression_text("1 + 2 * 3", &p);
    assert_op(e, AST_EXPRESSION_OPERATOR_ADD);
    assert_int_const(e->expression_operator.left_side, 1);
    assert_op(e->expression_operator.right_side, AST_EXPRESSION_OPERATOR_MUL);
    parsed_free(&p);

    e = parse_expression_text("1 * 2 + 3", &p);
    assert_op(e, AST_EXPRESSION_OPERATOR_ADD);
    assert_op(e->expression_operator.left_side, AST_EXPRESSION_OPERATOR_MUL);
    assert_int_const(e->expression_operator.right_side, 3);
    parsed_free(&p);
}

static void test_expr_left_associativity(void **state) {
    (void)state;
    parsed_t p;
    ast_node_t *e = parse_expression_text("10 - 3 - 2", &p);
    assert_op(e, AST_EXPRESSION_OPERATOR_SUB);
    assert_op(e->expression_operator.left_side, AST_EXPRESSION_OPERATOR_SUB);
    assert_int_const(e->expression_operator.right_side, 2);
    parsed_free(&p);

    e = parse_expression_text("8 / 4 / 2", &p);
    assert_op(e, AST_EXPRESSION_OPERATOR_DIV);
    assert_op(e->expression_operator.left_side, AST_EXPRESSION_OPERATOR_DIV);
    parsed_free(&p);
}

static void test_expr_parentheses_override_precedence(void **state) {
    (void)state;
    parsed_t p;
    ast_node_t *e = parse_expression_text("(1 + 2) * 3", &p);
    assert_op(e, AST_EXPRESSION_OPERATOR_MUL);
    assert_op(e->expression_operator.left_side, AST_EXPRESSION_OPERATOR_ADD);
    parsed_free(&p);

    e = parse_expression_text("((1))", &p);
    assert_int_const(e, 1);
    parsed_free(&p);
}

static void test_expr_relational_lower_than_arithmetic(void **state) {
    (void)state;
    parsed_t p;
    ast_node_t *e = parse_expression_text("1 + 2 < 3 * 4", &p);
    assert_op(e, AST_EXPRESSION_OPERATOR_SMALLER);
    assert_op(e->expression_operator.left_side, AST_EXPRESSION_OPERATOR_ADD);
    assert_op(e->expression_operator.right_side, AST_EXPRESSION_OPERATOR_MUL);
    parsed_free(&p);
}

static void test_expr_equality_lower_than_relational(void **state) {
    (void)state;
    parsed_t p;
    ast_node_t *e = parse_expression_text("1 < 2 == 3 > 4", &p);
    assert_op(e, AST_EXPRESSION_OPERATOR_EQUAL);
    assert_op(e->expression_operator.left_side, AST_EXPRESSION_OPERATOR_SMALLER);
    assert_op(e->expression_operator.right_side, AST_EXPRESSION_OPERATOR_BIGGER);
    parsed_free(&p);
}

static void test_expr_relational_operators_are_not_associative(void **state) {
    (void)state;
    // spec section 3: chaining of relational operators is not allowed
    ASSERT_SYNTAX_ERROR(PROLOG MAIN_HDR "  val a = 1 < 2 < 3\n");
    ASSERT_SYNTAX_ERROR(PROLOG MAIN_HDR "  val a = 1 >= 2 <= 3\n");
    ASSERT_SYNTAX_ERROR(PROLOG MAIN_HDR "  val a = 1 == 2 == 3\n");
    ASSERT_SYNTAX_ERROR(PROLOG MAIN_HDR "  val a = 1 != 2 == 3\n");
}

static void test_expr_unary_minus(void **state) {
    (void)state;
    parsed_t p;
    ast_node_t *e = parse_expression_text("-5", &p);
    assert_op(e, AST_EXPRESSION_OPERATOR_UNARY_MINUS);
    assert_null(e->expression_operator.left_side);
    assert_int_const(e->expression_operator.right_side, 5);
    parsed_free(&p);
}

static void test_expr_unary_plus(void **state) {
    (void)state;
    parsed_t p;
    ast_node_t *e = parse_expression_text("+5", &p);
    assert_op(e, AST_EXPRESSION_OPERATOR_UNARY_PLUS);
    parsed_free(&p);
}

static void test_expr_unary_binds_tighter_than_multiplication(void **state) {
    (void)state;
    parsed_t p;
    ast_node_t *e = parse_expression_text("-2 * 3", &p);
    assert_op(e, AST_EXPRESSION_OPERATOR_MUL);
    assert_op(e->expression_operator.left_side, AST_EXPRESSION_OPERATOR_UNARY_MINUS);
    parsed_free(&p);
}

static void test_expr_unary_after_binary_operator(void **state) {
    (void)state;
    parsed_t p;
    ast_node_t *e = parse_expression_text("2 * -3", &p);
    assert_op(e, AST_EXPRESSION_OPERATOR_MUL);
    assert_op(e->expression_operator.right_side, AST_EXPRESSION_OPERATOR_UNARY_MINUS);
    parsed_free(&p);

    e = parse_expression_text("2 - -3", &p);
    assert_op(e, AST_EXPRESSION_OPERATOR_SUB);
    assert_op(e->expression_operator.right_side, AST_EXPRESSION_OPERATOR_UNARY_MINUS);
    parsed_free(&p);
}

static void test_expr_unary_minus_of_parenthesis(void **state) {
    (void)state;
    parsed_t p;
    ast_node_t *e = parse_expression_text("-(1 + 2)", &p);
    assert_op(e, AST_EXPRESSION_OPERATOR_UNARY_MINUS);
    assert_op(e->expression_operator.right_side, AST_EXPRESSION_OPERATOR_ADD);
    parsed_free(&p);
}

static void test_expr_double_unary(void **state) {
    (void)state;
    parsed_t p;
    ast_node_t *e = parse_expression_text("- -1", &p);
    assert_op(e, AST_EXPRESSION_OPERATOR_UNARY_MINUS);
    assert_op(e->expression_operator.right_side, AST_EXPRESSION_OPERATOR_UNARY_MINUS);
    parsed_free(&p);
}

static void test_expr_function_call_without_arguments(void **state) {
    (void)state;
    parsed_t p;
    ast_node_t *e = parse_expression_text("readInt()", &p);
    assert_int_equal(e->type, AST_NODE_EXPRESSION_FUN_CALL);
    assert_string_equal(e->expression_fun_call.fun_name, "readInt");
    assert_int_equal(e->expression_fun_call.params_count, 0);
    assert_null(e->expression_fun_call.params);
    parsed_free(&p);
}

static void test_expr_function_call_with_arguments(void **state) {
    (void)state;
    parsed_t p;
    ast_node_t *e = parse_expression_text("f(1, x, \"s\")", &p);
    assert_int_equal(e->type, AST_NODE_EXPRESSION_FUN_CALL);
    assert_int_equal(e->expression_fun_call.params_count, 3);
    ast_node_t *a = e->expression_fun_call.params;
    assert_int_const(a, 1);
    assert_int_equal(a->next->type, AST_NODE_EXPRESSION_VAR);
    assert_int_equal(a->next->next->type, AST_NODE_CONST_STRING);
    assert_null(a->next->next->next);
    parsed_free(&p);
}

static void test_expr_function_call_with_expression_arguments(void **state) {
    (void)state;
    parsed_t p;
    ast_node_t *e = parse_expression_text("f(1 + 2, (3))", &p);
    assert_int_equal(e->expression_fun_call.params_count, 2);
    assert_op(e->expression_fun_call.params, AST_EXPRESSION_OPERATOR_ADD);
    assert_int_const(e->expression_fun_call.params->next, 3);
    parsed_free(&p);
}

static void test_expr_nested_function_calls(void **state) {
    (void)state;
    parsed_t p;
    ast_node_t *e = parse_expression_text("f(g(1), h(i(2), 3))", &p);
    assert_int_equal(e->expression_fun_call.params_count, 2);
    ast_node_t *h = e->expression_fun_call.params->next;
    assert_int_equal(h->type, AST_NODE_EXPRESSION_FUN_CALL);
    assert_int_equal(h->expression_fun_call.params_count, 2);
    parsed_free(&p);
}

static void test_expr_function_call_in_arithmetic(void **state) {
    (void)state;
    parsed_t p;
    ast_node_t *e = parse_expression_text("2 * f(1) + 1", &p);
    assert_op(e, AST_EXPRESSION_OPERATOR_ADD);
    assert_op(e->expression_operator.left_side, AST_EXPRESSION_OPERATOR_MUL);
    assert_int_equal(e->expression_operator.left_side->expression_operator.right_side->type,
                     AST_NODE_EXPRESSION_FUN_CALL);
    parsed_free(&p);
}

static void test_expr_multiline_arguments_with_trailing_comma(void **state) {
    (void)state;
    // spec 1.2: LF inside parentheses is whitespace, trailing comma allowed before ')' on new line
    ASSERT_PARSES(PROLOG MAIN_HDR "  print(\n    1,\n    )\n");
}

static void test_expr_multiline_arguments_without_trailing_comma(void **state) {
    (void)state;
    ASSERT_PARSES(PROLOG MAIN_HDR "  val a = f(1,\n    2)\ndef f(x: Int, y: Int): Int = x\n");
}

static void test_expr_syntax_errors(void **state) {
    (void)state;
    const char *bad[] = {"1 +", "* 2", "1 2",  "(1 + 2",  "1 + 2)", "()()", "1 + * 2", "(",
                         ")",   "f(1", "f(,)", "f(1,,2)", "1 +* 2", "val",  "1 = 2"};
    for (size_t i = 0; i < sizeof(bad) / sizeof(bad[0]); i++) {
        char src[512];
        snprintf(src, sizeof(src), PROLOG MAIN_HDR "  val a = %s\n", bad[i]);
        parsed_t p = parse_text(src);
        if (p.result.success) {
            print_error("expression '%s' was accepted, expected syntax error\n", bad[i]);
        }
        assert_false(p.result.success);
        assert_int_equal(p.result.error_code, PARSE_ERR_SYNTAX);
        parsed_free(&p);
    }
}

static void test_expr_empty_after_assign_is_error(void **state) {
    (void)state;
    ASSERT_SYNTAX_ERROR(PROLOG MAIN_HDR "  val a =\n");
}

static void test_expr_long_chain_does_not_crash(void **state) {
    (void)state;
    char src[8192];
    size_t len = (size_t)snprintf(src, sizeof(src), PROLOG MAIN_HDR "  val a = 1");
    for (int i = 0; i < 500; i++) {
        len += (size_t)snprintf(src + len, sizeof(src) - len, " + %d", i);
    }
    snprintf(src + len, sizeof(src) - len, "\n");
    ASSERT_PARSES(src);
}

static void test_expr_deep_parentheses(void **state) {
    (void)state;
    char src[2048];
    size_t len = (size_t)snprintf(src, sizeof(src), PROLOG MAIN_HDR "  val a = ");
    for (int i = 0; i < 100; i++) {
        src[len++] = '(';
    }
    src[len++] = '1';
    for (int i = 0; i < 100; i++) {
        src[len++] = ')';
    }
    src[len++] = '\n';
    src[len] = '\0';
    ASSERT_PARSES(src);
}

// ---------------------------------------------------------------------------
// Lexical errors reaching the parser, null arguments
// ---------------------------------------------------------------------------

static void test_lexical_error_terminates_with_code_1(void **state) {
    (void)state;
    ASSERT_LEX_ERROR(PROLOG MAIN_HDR "  val a = 1 $ 2\n");
    ASSERT_LEX_ERROR(PROLOG MAIN_HDR "  val a = \"unterminated\n");
    ASSERT_LEX_ERROR(PROLOG MAIN_HDR "\tval a = 1\n");
    ASSERT_LEX_ERROR(PROLOG MAIN_HDR "  val a = 007\n");
    ASSERT_LEX_ERROR(PROLOG MAIN_HDR "  val a = !1\n");
}

static void test_valid_program_has_exit_status_zero_in_child(void **state) {
    (void)state;
    assert_int_equal(parse_status_in_child(PROLOG MAIN_HDR "  print(1)\n"), 0);
}

static void test_parse_null_arguments(void **state) {
    (void)state;
    scanner_conf_t conf;
    FILE *f = open_source("");
    scanner_init(&conf, f);
    ast_node_t *root = NULL;

    parse_result_t r = parse(NULL, &conf);
    assert_false(r.success);
    assert_int_equal(r.error_code, PARSE_ERR_INTERNAL);

    r = parse(&root, NULL);
    assert_false(r.success);
    assert_int_equal(r.error_code, PARSE_ERR_INTERNAL);
    fclose(f);
}

static void test_failed_parse_returns_null_ast(void **state) {
    (void)state;
    parsed_t p = parse_text(PROLOG MAIN_HDR "  val x\n");
    assert_false(p.result.success);
    assert_null(p.root);
    assert_non_null(p.result.error_msg);
}

static void test_error_line_is_reported(void **state) {
    (void)state;
    parsed_t p = parse_text(PROLOG MAIN_HDR "  val x = 1\n  val y\n");
    assert_false(p.result.success);
    // error is on line 4 (the line with 'val y')
    assert_int_equal(p.result.error_line, 4);
}

int main(void) {
    const struct CMUnitTest tests[] = {
        T(test_minimal_program),
        T(test_program_root_is_block_with_import_first),
        T(test_prolog_may_be_preceded_by_comments_and_blank_lines),
        T(test_prolog_allows_spaces_between_tokens),
        T(test_missing_prolog_is_syntax_error),
        T(test_wrong_prolog_is_syntax_error),
        T(test_duplicate_prolog_is_syntax_error),
        T(test_empty_input_is_syntax_error),
        T(test_only_prolog_without_function_is_not_success),
        T(test_main_annotation_on_separate_line),
        T(test_main_annotation_on_same_line),
        T(test_main_can_have_any_identifier),
        T(test_functions_before_and_after_main),
        T(test_missing_main_is_not_a_syntax_error),
        T(test_duplicate_main_is_not_a_syntax_error),
        T(test_annotation_without_def_is_syntax_error),
        T(test_statement_at_top_level_is_syntax_error),
        T(test_function_without_parameters),
        T(test_function_parameters_are_parsed_in_order),
        T(test_return_type_may_be_omitted),
        T(test_all_return_types),
        T(test_one_line_function_body_is_wrapped_in_block),
        T(test_multiline_function_body),
        T(test_function_header_errors),
        T(test_function_parameter_errors),
        T(test_trailing_comma_in_parameters_requires_newline_form),
        T(test_trailing_comma_on_same_line_is_error),
        T(test_one_line_function_with_trailing_tokens_is_error),
        T(test_val_declaration),
        T(test_var_declaration_is_mutable),
        T(test_declaration_with_explicit_type),
        T(test_declaration_errors),
        T(test_assignment),
        T(test_assignment_errors),
        T(test_expression_statement_function_call),
        T(test_statements_keep_source_order),
        T(test_semicolon_separated_statements),
        T(test_two_statements_on_one_line_without_separator_is_error),
        T(test_return_with_value),
        T(test_return_without_value),
        T(test_return_with_call),
        T(test_if_else_statement),
        T(test_if_without_else_is_syntax_error_in_base_assignment),
        T(test_if_errors),
        T(test_while_statement),
        T(test_while_errors),
        T(test_nested_blocks),
        T(test_statement_after_dedent_belongs_to_outer_block),
        T(test_blank_lines_and_comments_inside_blocks),
        T(test_unit_literal_as_block_body),
        T(test_empty_function_block_is_error),
        T(test_missing_indentation_is_syntax_error),
        T(test_keyword_at_statement_start_is_error),
        T(test_main_body_ends_at_eof_without_newline),
        T(test_expr_literals),
        T(test_expr_variable),
        T(test_expr_each_binary_operator_maps_correctly),
        T(test_expr_mul_binds_tighter_than_add),
        T(test_expr_left_associativity),
        T(test_expr_parentheses_override_precedence),
        T(test_expr_relational_lower_than_arithmetic),
        T(test_expr_equality_lower_than_relational),
        T(test_expr_relational_operators_are_not_associative),
        T(test_expr_unary_minus),
        T(test_expr_unary_plus),
        T(test_expr_unary_binds_tighter_than_multiplication),
        T(test_expr_unary_after_binary_operator),
        T(test_expr_unary_minus_of_parenthesis),
        T(test_expr_double_unary),
        T(test_expr_function_call_without_arguments),
        T(test_expr_function_call_with_arguments),
        T(test_expr_function_call_with_expression_arguments),
        T(test_expr_nested_function_calls),
        T(test_expr_function_call_in_arithmetic),
        T(test_expr_multiline_arguments_with_trailing_comma),
        T(test_expr_multiline_arguments_without_trailing_comma),
        T(test_expr_syntax_errors),
        T(test_expr_empty_after_assign_is_error),
        T(test_expr_long_chain_does_not_crash),
        T(test_expr_deep_parentheses),
        T(test_lexical_error_terminates_with_code_1),
        T(test_valid_program_has_exit_status_zero_in_child),
        T(test_parse_null_arguments),
        T(test_failed_parse_returns_null_ast),
        T(test_error_line_is_reported),
    };
    return cmocka_run_group_tests_name("parser", tests, NULL, NULL);
}
