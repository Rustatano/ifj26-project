/*
-------------------------------------------------------
Name of project: Translator IFJ26
File: test_codegen.c

Description: Unit tests (cmocka) of the code generator (codegen.c).
             Hand-built, already type-annotated ASTs are translated to
             a temporary file and the produced IFJcode26 text is checked.
             Opcodes are also validated against the instruction set of
             the specification (section 7.4).
-------------------------------------------------------
*/

#define _GNU_SOURCE
#include "ast.h"
#include "codegen.h"
#include "test_helpers.h"
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define T(fn) cmocka_unit_test_setup_teardown(fn, test_setup_timeout, test_teardown_timeout)

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

// Runs codegen on root and returns the generated text (malloc'd). *ok receives codegen's result.
static char *generate(ast_node_t *root, bool *ok) {
    char path[] = "/tmp/ifj26_codegen_test_XXXXXX";
    int fd = mkstemp(path);
    assert_true(fd >= 0);
    close(fd);

    bool result = codegen(root, path);
    if (ok != NULL) {
        *ok = result;
    }

    FILE *f = fopen(path, "r");
    assert_non_null(f);
    char *buf = calloc(1, 1 << 16);
    size_t n = fread(buf, 1, (1 << 16) - 1, f);
    buf[n] = '\0';
    fclose(f);
    unlink(path);
    return buf;
}

static int count_substr(const char *hay, const char *needle) {
    int c = 0;
    for (const char *p = hay; (p = strstr(p, needle)) != NULL; p += strlen(needle)) {
        c++;
    }
    return c;
}

#define ASSERT_HAS(text, needle)                                                                   \
    do {                                                                                           \
        if (strstr((text), (needle)) == NULL) {                                                    \
            print_error("missing '%s' in generated code:\n%s\n", (needle), (text));                \
        }                                                                                          \
        assert_non_null(strstr((text), (needle)));                                                 \
    } while (0)

#define ASSERT_HAS_NOT(text, needle)                                                               \
    do {                                                                                           \
        if (strstr((text), (needle)) != NULL) {                                                    \
            print_error("unexpected '%s' in generated code:\n%s\n", (needle), (text));             \
        }                                                                                          \
        assert_null(strstr((text), (needle)));                                                     \
    } while (0)

static ast_node_t *int_node(int v) {
    ast_node_t *n = ast_const_int_create(v, 1);
    n->result_type = SYM_DATATYPE_INT;
    return n;
}

static ast_node_t *double_node(double v) {
    ast_node_t *n = ast_const_double_create(v, 1);
    n->result_type = SYM_DATATYPE_DOUBLE;
    return n;
}

static ast_node_t *string_node(const char *s) {
    ast_node_t *n = ast_const_string_create(strdup(s), 1);
    n->result_type = SYM_DATATYPE_STRING;
    return n;
}

static ast_node_t *var_use(const char *name, int id, symtable_datatype_t t) {
    return ast_expression_var_create(strdup(name), id, t, 1);
}

static ast_node_t *binop(ast_expression_operator_t op, ast_node_t *l, ast_node_t *r,
                         symtable_datatype_t result) {
    ast_node_t *n = ast_expression_operation_create(op, l, r, 1);
    n->result_type = result;
    return n;
}

// Wraps a statement list into root block: { import, <stmt> }
static ast_node_t *root_with(ast_node_t *stmt) {
    ast_node_t *root = ast_block_create(1);
    ast_block_append(root, ast_import_create("ifj26.*", 1));
    ast_block_append(root, stmt);
    return root;
}

static ast_node_t *block_of(ast_node_t *a, ast_node_t *b) {
    ast_node_t *blk = ast_block_create(1);
    ast_block_append(blk, a);
    if (b != NULL) {
        ast_block_append(blk, b);
    }
    return blk;
}

static ast_node_t *fun_def(const char *name, symtable_fun_param_t *params, ast_node_t *body) {
    return ast_fun_definition_create(strdup(name), params, SYM_DATATYPE_INT, body, 1);
}

static ast_node_t *main_def(ast_node_t *body) {
    return ast_main_create(strdup("main"), NULL, SYM_DATATYPE_UNIT, body, 1);
}

static ast_node_t *print_call(ast_node_t *arg) {
    ast_node_t *c = ast_expression_fun_call_create(strdup("print"), arg, 1, 1);
    c->result_type = SYM_DATATYPE_UNIT;
    return c;
}

// Generates code for a program with one ordinary function "f" whose body is just `stmt`.
// (An ordinary function is used on purpose, so that each test checks one feature and does not
// depend on the generation of the @main function body, which has its own tests.)
static char *generate_main_with(ast_node_t *stmt) {
    ast_node_t *root = root_with(fun_def("f", NULL, block_of(stmt, NULL)));
    bool ok = false;
    char *code = generate(root, &ok);
    assert_true(ok);
    ast_dispose(&root);
    return code;
}

// Valid IFJcode26 opcodes (spec 7.4) - upper-case.
static const char *const VALID_OPCODES[] = {
    "MOVE",       "CREATEFRAME", "PUSHFRAME", "POPFRAME",   "DEFVAR",     "CALL",      "RETURN",
    "PUSHS",      "POPS",        "CLEARS",    "ADD",        "SUB",        "MUL",       "DIV",
    "IDIV",       "ADDS",        "SUBS",      "MULS",       "DIVS",       "IDIVS",     "LT",
    "GT",         "EQ",          "LTS",       "GTS",        "EQS",        "AND",       "OR",
    "NOT",        "ANDS",        "ORS",       "NOTS",       "INT2FLOAT",  "FLOAT2INT", "INT2CHAR",
    "STRI2INT",   "FLOAT2STR",   "INT2STR",   "INT2FLOATS", "FLOAT2INTS", "INT2CHARS", "STRI2INTS",
    "FLOAT2STRS", "INT2STRS",    "READ",      "WRITE",      "CONCAT",     "STRLEN",    "GETCHAR",
    "SETCHAR",    "NEWARRAY",    "ARRAYGET",  "ARRAYSET",   "TYPE",       "ISINT",     "TYPES",
    "ISINTS",     "LABEL",       "JUMP",      "JUMPIFEQ",   "JUMPIFNEQ",  "JUMPIFEQS", "JUMPIFNEQS",
    "EXIT",       "BREAK",       "DPRINT",
};

static bool is_valid_opcode(const char *op) {
    for (size_t i = 0; i < sizeof(VALID_OPCODES) / sizeof(VALID_OPCODES[0]); i++) {
        if (strcasecmp(op, VALID_OPCODES[i]) == 0) {
            return true;
        }
    }
    return false;
}

// Checks that every line is either the header or a valid instruction.
static void assert_only_valid_opcodes(const char *code) {
    char *copy = strdup(code);
    char *save = NULL;
    for (char *line = strtok_r(copy, "\n", &save); line != NULL;
         line = strtok_r(NULL, "\n", &save)) {
        if (line[0] == '.' || line[0] == '#' || line[0] == '\0') {
            continue;
        }
        char op[64];
        if (sscanf(line, "%63s", op) != 1) {
            continue;
        }
        if (!is_valid_opcode(op)) {
            print_error("invalid IFJcode26 opcode '%s' in line: %s\n", op, line);
        }
        assert_true(is_valid_opcode(op));
    }
    free(copy);
}

// ---------------------------------------------------------------------------
// Skeleton of the output
// ---------------------------------------------------------------------------

static void test_header_is_first_line(void **state) {
    (void)state;
    ast_node_t *root = ast_block_create(1);
    bool ok = false;
    char *code = generate(root, &ok);
    assert_true(ok);
    assert_true(strncmp(code, ".IFJcode26\n", 11) == 0);
    free(code);
    ast_dispose(&root);
}

static void test_skeleton_defines_temporaries_and_jumps_to_main(void **state) {
    (void)state;
    ast_node_t *root = root_with(main_def(block_of(print_call(int_node(1)), NULL)));
    char *code = generate(root, NULL);
    for (int i = 0; i < 6; i++) {
        char needle[32];
        snprintf(needle, sizeof(needle), "DEFVAR GF@tmp%d", i);
        ASSERT_HAS(code, needle);
    }
    ASSERT_HAS(code, "JUMP $main\n");
    ASSERT_HAS(code, "LABEL $EXIT40\nEXIT int@40\n");
    free(code);
    ast_dispose(&root);
}

static void test_jump_to_main_comes_before_function_labels(void **state) {
    (void)state;
    ast_node_t *root = root_with(main_def(block_of(print_call(int_node(1)), NULL)));
    char *code = generate(root, NULL);
    char *jump = strstr(code, "JUMP $main");
    char *label = strstr(code, "LABEL $main");
    assert_non_null(jump);
    assert_non_null(label);
    assert_true(jump < label);
    free(code);
    ast_dispose(&root);
}

static void test_codegen_to_unwritable_path_fails(void **state) {
    (void)state;
    ast_node_t *root = ast_block_create(1);
    assert_false(codegen(root, "/nonexistent_dir_ifj26/out.code"));
    ast_dispose(&root);
}

static void test_main_function_body_is_generated(void **state) {
    (void)state;
    ast_node_t *root = root_with(main_def(block_of(print_call(int_node(42)), NULL)));
    bool ok = false;
    char *code = generate(root, &ok);
    assert_true(ok);
    ASSERT_HAS(code, "LABEL $main\n");
    ASSERT_HAS(code, "PUSHS int@42\n");
    ASSERT_HAS(code, "WRITE GF@tmp0\n");
    ASSERT_HAS(code, "EXIT int@0\n");
    free(code);
    ast_dispose(&root);
}

static void test_main_ends_with_exit_zero_before_exit40_label(void **state) {
    (void)state;
    ast_node_t *root = root_with(main_def(block_of(print_call(int_node(1)), NULL)));
    char *code = generate(root, NULL);
    char *exit0 = strstr(code, "EXIT int@0");
    char *label40 = strstr(code, "LABEL $EXIT40");
    assert_non_null(exit0);
    assert_non_null(label40);
    assert_true(exit0 < label40);
    free(code);
    ast_dispose(&root);
}

// ---------------------------------------------------------------------------
// Constants
// ---------------------------------------------------------------------------

static void test_int_constants(void **state) {
    (void)state;
    char *code = generate_main_with(print_call(int_node(-7)));
    ASSERT_HAS(code, "PUSHS int@-7\n");
    free(code);
}

static void test_double_constant_uses_hex_float_format(void **state) {
    (void)state;
    char *code = generate_main_with(print_call(double_node(0.5)));
    ASSERT_HAS(code, "PUSHS float@0x1p-1\n"); // printf("%a", 0.5)
    free(code);
}

static void test_string_constant(void **state) {
    (void)state;
    char *code = generate_main_with(print_call(string_node("hello")));
    ASSERT_HAS(code, "PUSHS string@hello\n");
    free(code);
}

static void test_string_constant_escapes_space_and_special_characters(void **state) {
    (void)state;
    // spec 7.3: characters 000-032, 035 (#) and 092 (\) must be written as \xyz
    char *code = generate_main_with(print_call(string_node("a b#c\\d\ne")));
    ASSERT_HAS(code, "PUSHS string@a\\032b\\035c\\092d\\010e\n");
    free(code);
}

static void test_empty_string_constant(void **state) {
    (void)state;
    char *code = generate_main_with(print_call(string_node("")));
    ASSERT_HAS(code, "PUSHS string@\n");
    free(code);
}

static void test_unit_constant_and_print_of_unit(void **state) {
    (void)state;
    ast_node_t *unit = ast_const_unit_create(1);
    unit->result_type = SYM_DATATYPE_UNIT;
    char *code = generate_main_with(print_call(unit));
    ASSERT_HAS(code, "PUSHS nil@nil\n");
    ASSERT_HAS(code, "MOVE GF@tmp0 string@()\n"); // () is printed as "()"
    free(code);
}

// ---------------------------------------------------------------------------
// Variables
// ---------------------------------------------------------------------------

static void test_variable_declaration(void **state) {
    (void)state;
    ast_node_t *d =
        ast_var_declaration_create(strdup("x"), 3, SYM_DATATYPE_INT, false, int_node(5), 1);
    char *code = generate_main_with(d);
    ASSERT_HAS(code, "DEFVAR LF@x$3\n");
    ASSERT_HAS(code, "PUSHS int@5\nPOPS LF@x$3\n");
    free(code);
}

static void test_assignment(void **state) {
    (void)state;
    ast_node_t *a = ast_assignment_create(strdup("x"), 3, int_node(9), 1);
    char *code = generate_main_with(a);
    ASSERT_HAS(code, "PUSHS int@9\nPOPS LF@x$3\n");
    ASSERT_HAS_NOT(code, "DEFVAR LF@x$3");
    free(code);
}

static void test_variable_use(void **state) {
    (void)state;
    char *code = generate_main_with(print_call(var_use("x", 3, SYM_DATATYPE_INT)));
    ASSERT_HAS(code, "PUSHS LF@x$3\n");
    free(code);
}

static void test_main_frame_exists_for_local_variables(void **state) {
    (void)state;
    // LF@ is used for locals, so main must create and push a frame before DEFVAR LF@...
    ast_node_t *d =
        ast_var_declaration_create(strdup("x"), 0, SYM_DATATYPE_INT, false, int_node(1), 1);
    ast_node_t *root = root_with(main_def(block_of(d, NULL)));
    char *code = generate(root, NULL);
    ast_dispose(&root);
    char *frame = strstr(code, "PUSHFRAME");
    char *defvar = strstr(code, "DEFVAR LF@x$0");
    assert_non_null(defvar);
    assert_non_null(frame);
    assert_true(frame < defvar);
    free(code);
}

// ---------------------------------------------------------------------------
// Operators
// ---------------------------------------------------------------------------

static char *gen_binary(ast_expression_operator_t op, symtable_datatype_t operand_type,
                        ast_node_t *l, ast_node_t *r, symtable_datatype_t result) {
    (void)operand_type;
    return generate_main_with(print_call(binop(op, l, r, result)));
}

static void test_int_addition_with_overflow_check(void **state) {
    (void)state;
    char *code = gen_binary(AST_EXPRESSION_OPERATOR_ADD, SYM_DATATYPE_INT, int_node(1), int_node(2),
                            SYM_DATATYPE_INT);
    ASSERT_HAS(code, "PUSHS int@1\nPUSHS int@2\nADDS\n");
    ASSERT_HAS(code, "CALL $OVERFLOW_CHECK\n");
    ASSERT_HAS(code, "LABEL $OVERFLOW_CHECK\n");
    free(code);
}

static void test_double_addition_has_no_overflow_check(void **state) {
    (void)state;
    char *code = gen_binary(AST_EXPRESSION_OPERATOR_ADD, SYM_DATATYPE_DOUBLE, double_node(1),
                            double_node(2), SYM_DATATYPE_DOUBLE);
    ASSERT_HAS(code, "ADDS\n");
    ASSERT_HAS_NOT(code, "OVERFLOW_CHECK");
    free(code);
}

static void test_overflow_routine_not_emitted_when_unused(void **state) {
    (void)state;
    char *code = generate_main_with(print_call(int_node(1)));
    ASSERT_HAS_NOT(code, "OVERFLOW");
    free(code);
}

static void test_overflow_routine_emitted_once(void **state) {
    (void)state;
    ast_node_t *e =
        binop(AST_EXPRESSION_OPERATOR_ADD,
              binop(AST_EXPRESSION_OPERATOR_MUL, int_node(1), int_node(2), SYM_DATATYPE_INT),
              int_node(3), SYM_DATATYPE_INT);
    char *code = generate_main_with(print_call(e));
    assert_int_equal(count_substr(code, "LABEL $OVERFLOW_CHECK"), 1);
    assert_true(count_substr(code, "CALL $OVERFLOW_CHECK") >= 2);
    free(code);
}

static void test_subtraction_and_multiplication(void **state) {
    (void)state;
    char *code = gen_binary(AST_EXPRESSION_OPERATOR_SUB, SYM_DATATYPE_INT, int_node(1), int_node(2),
                            SYM_DATATYPE_INT);
    ASSERT_HAS(code, "SUBS\n");
    free(code);
    code = gen_binary(AST_EXPRESSION_OPERATOR_MUL, SYM_DATATYPE_INT, int_node(1), int_node(2),
                      SYM_DATATYPE_INT);
    ASSERT_HAS(code, "MULS\n");
    free(code);
}

static void test_int_division_uses_idivs_and_double_uses_divs(void **state) {
    (void)state;
    char *code = gen_binary(AST_EXPRESSION_OPERATOR_DIV, SYM_DATATYPE_INT, int_node(7), int_node(2),
                            SYM_DATATYPE_INT);
    ASSERT_HAS(code, "IDIVS\n");
    ASSERT_HAS_NOT(code, "\nDIVS\n");
    free(code);
    code = gen_binary(AST_EXPRESSION_OPERATOR_DIV, SYM_DATATYPE_DOUBLE, double_node(7),
                      double_node(2), SYM_DATATYPE_DOUBLE);
    ASSERT_HAS(code, "\nDIVS\n");
    ASSERT_HAS_NOT(code, "IDIVS");
    free(code);
}

static void test_string_concatenation(void **state) {
    (void)state;
    char *code = gen_binary(AST_EXPRESSION_OPERATOR_ADD, SYM_DATATYPE_STRING, string_node("a"),
                            string_node("b"), SYM_DATATYPE_STRING);
    ASSERT_HAS(code, "CONCAT");
    ASSERT_HAS_NOT(code, "ADDS");
    free(code);
}

static void test_concatenation_keeps_operand_order(void **state) {
    (void)state;
    // stack: [a, b] -> POPS tmp0 (= b), POPS tmp1 (= a), CONCAT result tmp1 tmp0 (= a + b)
    char *code = gen_binary(AST_EXPRESSION_OPERATOR_ADD, SYM_DATATYPE_STRING, string_node("a"),
                            string_node("b"), SYM_DATATYPE_STRING);
    ASSERT_HAS(code, "POPS GF@tmp0\nPOPS GF@tmp1\nCONCAT GF@tmp0 GF@tmp1 GF@tmp0\n");
    free(code);
}

static void test_relational_operators(void **state) {
    (void)state;
    struct {
        ast_expression_operator_t op;
        const char *expected;
    } cases[] = {
        {AST_EXPRESSION_OPERATOR_SMALLER, "LTS\n"},
        {AST_EXPRESSION_OPERATOR_BIGGER, "GTS\n"},
        {AST_EXPRESSION_OPERATOR_EQUAL, "EQS\n"},
        {AST_EXPRESSION_OPERATOR_NOT_EQUAL, "EQS\nNOTS\n"},
    };
    for (size_t i = 0; i < sizeof(cases) / sizeof(cases[0]); i++) {
        char *code =
            gen_binary(cases[i].op, SYM_DATATYPE_INT, int_node(1), int_node(2), SYM_DATATYPE_INT);
        ASSERT_HAS(code, cases[i].expected);
        free(code);
    }
}

static void test_less_equal_keeps_operand_order(void **state) {
    (void)state;
    // a <= b  ==  (a < b) or (a == b); operands must be restored to the original order each time:
    // stack [a, b]: POPS tmp0(b) POPS tmp1(a); PUSHS tmp1 PUSHS tmp0 LTS ...
    char *code = gen_binary(AST_EXPRESSION_OPERATOR_SMALLER_EQUAL, SYM_DATATYPE_INT, int_node(1),
                            int_node(2), SYM_DATATYPE_INT);
    ASSERT_HAS(code, "POPS GF@tmp0\nPOPS GF@tmp1\nPUSHS GF@tmp1\nPUSHS GF@tmp0\nLTS\n");
    ASSERT_HAS(code, "PUSHS GF@tmp1\nPUSHS GF@tmp0\nEQS\nORS\n");
    free(code);
}

static void test_greater_equal_keeps_operand_order(void **state) {
    (void)state;
    char *code = gen_binary(AST_EXPRESSION_OPERATOR_BIGGER_EQUAL, SYM_DATATYPE_INT, int_node(1),
                            int_node(2), SYM_DATATYPE_INT);
    ASSERT_HAS(code, "POPS GF@tmp0\nPOPS GF@tmp1\nPUSHS GF@tmp1\nPUSHS GF@tmp0\nGTS\n");
    ASSERT_HAS(code, "PUSHS GF@tmp1\nPUSHS GF@tmp0\nEQS\nORS\n");
    free(code);
}

static void test_unary_minus_is_generated(void **state) {
    (void)state;
    ast_node_t *e =
        ast_expression_operation_create(AST_EXPRESSION_OPERATOR_UNARY_MINUS, NULL, int_node(5), 1);
    e->result_type = SYM_DATATYPE_INT;
    bool ok = false;
    ast_node_t *root = root_with(fun_def("f", NULL, block_of(print_call(e), NULL)));
    char *code = generate(root, &ok);
    assert_true(ok);
    // -x must be computed as 0 - x (or an equivalent); the operand must appear
    ASSERT_HAS(code, "PUSHS int@5\n");
    ASSERT_HAS(code, "SUBS\n");
    free(code);
    ast_dispose(&root);
}

static void test_unary_plus_is_generated(void **state) {
    (void)state;
    ast_node_t *e =
        ast_expression_operation_create(AST_EXPRESSION_OPERATOR_UNARY_PLUS, NULL, int_node(5), 1);
    e->result_type = SYM_DATATYPE_INT;
    bool ok = false;
    ast_node_t *root = root_with(fun_def("f", NULL, block_of(print_call(e), NULL)));
    char *code = generate(root, &ok);
    assert_true(ok);
    ASSERT_HAS(code, "PUSHS int@5\n");
    free(code);
    ast_dispose(&root);
}

static void test_int_to_double_conversion_node(void **state) {
    (void)state;
    ast_node_t *conv = ast_block_create(1);
    conv->type = AST_NODE_CONVERT;
    conv->result_type = SYM_DATATYPE_DOUBLE;
    conv->block.first_statement = int_node(3);
    char *code = generate_main_with(print_call(conv));
    ASSERT_HAS(code, "PUSHS int@3\nINT2FLOATS\n");
    free(code);
}

// ---------------------------------------------------------------------------
// Control flow
// ---------------------------------------------------------------------------

static ast_node_t *cond_lt(void) {
    return binop(AST_EXPRESSION_OPERATOR_SMALLER, int_node(1), int_node(2), SYM_DATATYPE_INT);
}

static void test_if_else_labels(void **state) {
    (void)state;
    ast_node_t *i = ast_if_create(cond_lt(), block_of(print_call(int_node(1)), NULL),
                                  block_of(print_call(int_node(2)), NULL), 1);
    char *code = generate_main_with(i);
    ASSERT_HAS(code, "LABEL $IF_TRUE$");
    ASSERT_HAS(code, "LABEL $IF_FALSE$");
    ASSERT_HAS(code, "LABEL $IF_END$");
    ASSERT_HAS(code, "PUSHS bool@true\nJUMPIFEQS $IF_TRUE$");
    free(code);
}

static void test_two_ifs_have_unique_labels(void **state) {
    (void)state;
    ast_node_t *i1 = ast_if_create(cond_lt(), block_of(print_call(int_node(1)), NULL),
                                   block_of(print_call(int_node(2)), NULL), 1);
    ast_node_t *i2 = ast_if_create(cond_lt(), block_of(print_call(int_node(3)), NULL),
                                   block_of(print_call(int_node(4)), NULL), 1);
    ast_node_t *root = root_with(fun_def("f", NULL, block_of(i1, i2)));
    bool ok = false;
    char *code = generate(root, &ok);
    assert_true(ok);
    // each label may be defined only once (spec 7.4.8: redefinition is error 52)
    assert_int_equal(count_substr(code, "LABEL $IF_TRUE$"), 2);
    assert_int_equal(count_substr(code, "JUMPIFEQS $IF_TRUE$"), 2);
    free(code);
    ast_dispose(&root);
}

static void test_nested_ifs_have_unique_labels(void **state) {
    (void)state;
    ast_node_t *inner = ast_if_create(cond_lt(), block_of(print_call(int_node(1)), NULL),
                                      block_of(print_call(int_node(2)), NULL), 1);
    ast_node_t *outer =
        ast_if_create(cond_lt(), block_of(inner, NULL), block_of(print_call(int_node(3)), NULL), 1);
    char *code = generate_main_with(outer);
    // Every LABEL line must be unique
    char *copy = strdup(code);
    char *save = NULL;
    char *labels[64];
    int n = 0;
    for (char *line = strtok_r(copy, "\n", &save); line != NULL && n < 64;
         line = strtok_r(NULL, "\n", &save)) {
        if (strncmp(line, "LABEL ", 6) == 0) {
            for (int i = 0; i < n; i++) {
                if (strcmp(labels[i], line) == 0) {
                    print_error("label defined twice: %s\n%s\n", line, code);
                }
                assert_string_not_equal(labels[i], line);
            }
            labels[n++] = line;
        }
    }
    free(copy);
    free(code);
}

static void test_while_loop(void **state) {
    (void)state;
    ast_node_t *body = block_of(print_call(int_node(1)), NULL);
    ast_node_t *w = ast_while_create(cond_lt(), body, 1);
    char *code = generate_main_with(w);
    ASSERT_HAS(code, "LABEL $WHILE$");
    ASSERT_HAS(code, "JUMPIFNEQS $WHILE_END$");
    ASSERT_HAS(code, "JUMP $WHILE$");
    free(code);
}

static void test_while_condition_is_checked_before_the_first_iteration(void **state) {
    (void)state;
    // while c do body must not execute body when c is false at the start:
    // the condition must be evaluated (and may exit the loop) before the body.
    ast_node_t *body = block_of(print_call(string_node("BODY")), NULL);
    ast_node_t *w = ast_while_create(cond_lt(), body, 1);
    char *code = generate_main_with(w);
    char *body_pos = strstr(code, "string@BODY");
    char *cond_pos = strstr(code, "LTS");
    assert_non_null(body_pos);
    assert_non_null(cond_pos);
    if (!(cond_pos < body_pos)) {
        print_error("loop body is generated before the condition is tested:\n%s\n", code);
    }
    assert_true(cond_pos < body_pos);
    free(code);
}

static void test_while_as_last_statement_does_not_crash(void **state) {
    (void)state;
    // codegen of WHILE reads node->next->block...: it must cope with a loop that has no successor
    ast_node_t *w = ast_while_create(cond_lt(), block_of(print_call(int_node(1)), NULL), 1);
    ast_node_t *root = root_with(fun_def("f", NULL, block_of(w, NULL)));
    bool ok = false;
    char *code = generate(root, &ok);
    assert_true(ok);
    free(code);
    ast_dispose(&root);
}

static void test_variable_in_loop_body_is_defined_once(void **state) {
    (void)state;
    ast_node_t *decl =
        ast_var_declaration_create(strdup("t"), 7, SYM_DATATYPE_INT, false, int_node(1), 1);
    ast_node_t *w = ast_while_create(cond_lt(), block_of(decl, NULL), 1);
    char *code = generate_main_with(w);
    assert_int_equal(count_substr(code, "DEFVAR LF@t$7"), 1);
    free(code);
}

// ---------------------------------------------------------------------------
// Functions
// ---------------------------------------------------------------------------

static symtable_fun_param_t *prm(const char *name, int id, symtable_fun_param_t *next) {
    symtable_fun_param_t *p = calloc(1, sizeof(*p));
    p->name = strdup(name);
    p->unique_id = id;
    p->type = SYM_DATATYPE_INT;
    p->next = next;
    return p;
}

static void test_function_definition_prologue(void **state) {
    (void)state;
    ast_node_t *f = fun_def("foo", prm("a", 1, prm("b", 2, NULL)),
                            block_of(ast_return_create(int_node(1), 1), NULL));
    ast_node_t *root = ast_block_create(1);
    ast_block_append(root, ast_import_create("ifj26.*", 1));
    ast_block_append(root, f);
    ast_block_append(root, main_def(block_of(print_call(int_node(1)), NULL)));
    char *code = generate(root, NULL);
    ASSERT_HAS(code, "LABEL foo\nCREATEFRAME\nPUSHFRAME\n");
    ASSERT_HAS(code, "DEFVAR LF@a$1\n");
    ASSERT_HAS(code, "DEFVAR LF@b$2\n");
    free(code);
    ast_dispose(&root);
}

static void test_function_parameters_are_popped_in_reverse_order(void **state) {
    (void)state;
    ast_node_t *f = fun_def("foo", prm("a", 1, prm("b", 2, NULL)),
                            block_of(ast_return_create(int_node(1), 1), NULL));
    ast_node_t *root = ast_block_create(1);
    ast_block_append(root, ast_import_create("ifj26.*", 1));
    ast_block_append(root, f);
    ast_block_append(root, main_def(block_of(print_call(int_node(1)), NULL)));
    char *code = generate(root, NULL);
    // arguments are pushed a, b -> b is on the top and must be popped first
    ASSERT_HAS(code, "POPS LF@b$2\nPOPS LF@a$1\n");
    free(code);
    ast_dispose(&root);
}

static void test_return_pops_frame_and_returns(void **state) {
    (void)state;
    ast_node_t *f = fun_def("foo", NULL, block_of(ast_return_create(int_node(1), 1), NULL));
    ast_node_t *root = ast_block_create(1);
    ast_block_append(root, ast_import_create("ifj26.*", 1));
    ast_block_append(root, f);
    ast_block_append(root, main_def(block_of(print_call(int_node(1)), NULL)));
    char *code = generate(root, NULL);
    ASSERT_HAS(code, "PUSHS int@1\nPOPFRAME\nRETURN\n");
    free(code);
    ast_dispose(&root);
}

static void test_function_without_explicit_return_still_returns(void **state) {
    (void)state;
    // spec 2.3: the value of the last expression is the return value, so the generated function
    // must end with POPFRAME/RETURN even without a `return` statement (otherwise execution falls
    // through)
    ast_node_t *f = fun_def("foo", NULL, block_of(int_node(5), NULL));
    ast_node_t *root = ast_block_create(1);
    ast_block_append(root, ast_import_create("ifj26.*", 1));
    ast_block_append(root, f);
    ast_block_append(root, main_def(block_of(print_call(int_node(1)), NULL)));
    char *code = generate(root, NULL);
    char *foo = strstr(code, "LABEL foo");
    char *main_label = strstr(code, "LABEL $main");
    assert_non_null(foo);
    assert_non_null(main_label);
    // RETURN must appear between the two labels (functions are emitted in source order)
    char *ret = strstr(foo, "RETURN");
    if (ret == NULL || ret > main_label) {
        print_error("function without return statement has no RETURN:\n%s\n", code);
    }
    assert_true(ret != NULL && ret < main_label);
    free(code);
    ast_dispose(&root);
}

static void test_user_function_call(void **state) {
    (void)state;
    ast_node_t *args = int_node(1);
    args->next = int_node(2);
    ast_node_t *c = ast_expression_fun_call_create(strdup("foo"), args, 2, 1);
    c->result_type = SYM_DATATYPE_INT;
    char *code = generate_main_with(print_call(c));
    ASSERT_HAS(code, "PUSHS int@1\nPUSHS int@2\nCALL foo\n");
    free(code);
}

static void test_function_label_cannot_clash_with_internal_labels(void **state) {
    (void)state;
    // user function named "main" in IFJ26 is allowed for non-@main functions? Not required.
    // A user function named e.g. "tmp0" must not clash with GF@tmp0 (different namespace) - just no
    // crash.
    ast_node_t *f = fun_def("tmp0", NULL, block_of(ast_return_create(int_node(1), 1), NULL));
    ast_node_t *root = ast_block_create(1);
    ast_block_append(root, ast_import_create("ifj26.*", 1));
    ast_block_append(root, f);
    ast_block_append(root, main_def(block_of(print_call(int_node(1)), NULL)));
    bool ok = false;
    char *code = generate(root, &ok);
    assert_true(ok);
    free(code);
    ast_dispose(&root);
}

// ---------------------------------------------------------------------------
// Builtin functions
// ---------------------------------------------------------------------------

static char *gen_builtin(const char *name, ast_node_t *args, int count, symtable_datatype_t ret) {
    ast_node_t *c = ast_expression_fun_call_create(strdup(name), args, count, 1);
    c->result_type = ret;
    return generate_main_with(print_call(c));
}

static void test_builtin_read_functions(void **state) {
    (void)state;
    char *code = gen_builtin("readInt", NULL, 0, SYM_DATATYPE_INT);
    ASSERT_HAS(code, "READ GF@tmp0 int\n");
    free(code);
    code = gen_builtin("readDouble", NULL, 0, SYM_DATATYPE_DOUBLE);
    ASSERT_HAS(code, "READ GF@tmp0 float\n");
    free(code);
    code = gen_builtin("readLine", NULL, 0, SYM_DATATYPE_STRING);
    ASSERT_HAS(code, "READ GF@tmp0 string\n");
    free(code);
}

static void test_builtin_read_int_returns_zero_on_error(void **state) {
    (void)state;
    // spec 4: readInt returns 0 at EOF / bad format, but READ yields nil -> generated code must
    // convert
    char *code = gen_builtin("readInt", NULL, 0, SYM_DATATYPE_INT);
    ASSERT_HAS(code, "int@0");
    free(code);
}

static void test_builtin_read_line_at_eof_returns_empty_string(void **state) {
    (void)state;
    char *code = gen_builtin("readLine", NULL, 0, SYM_DATATYPE_STRING);
    ASSERT_HAS(code, "string@");
    free(code);
}

static void test_builtin_to_str(void **state) {
    (void)state;
    char *code = gen_builtin("toStr", int_node(1), 1, SYM_DATATYPE_STRING);
    ASSERT_HAS(code, "INT2STRS\n");
    free(code);
    code = gen_builtin("toStr", double_node(1.5), 1, SYM_DATATYPE_STRING);
    ASSERT_HAS(code, "FLOAT2STRS\n");
    free(code);
    code = gen_builtin("toStr", string_node("s"), 1, SYM_DATATYPE_STRING);
    ASSERT_HAS_NOT(code, "INT2STRS");
    ASSERT_HAS_NOT(code, "FLOAT2STRS");
    free(code);
}

static void test_builtin_to_int(void **state) {
    (void)state;
    char *code = gen_builtin("toInt", double_node(1.5), 1, SYM_DATATYPE_INT);
    ASSERT_HAS(code, "FLOAT2INTS\n");
    free(code);
}

static void test_builtin_str_length(void **state) {
    (void)state;
    char *code = gen_builtin("strLength", string_node("abc"), 1, SYM_DATATYPE_INT);
    ASSERT_HAS(code, "STRLEN GF@tmp1 GF@tmp0\n");
    free(code);
}

static void test_builtin_chr(void **state) {
    (void)state;
    char *code = gen_builtin("chr", int_node(65), 1, SYM_DATATYPE_STRING);
    ASSERT_HAS(code, "INT2CHARS\n");
    free(code);
}

static void test_builtin_ord_uses_valid_instruction(void **state) {
    (void)state;
    ast_node_t *args = string_node("abc");
    args->next = int_node(1);
    char *code = gen_builtin("ord", args, 2, SYM_DATATYPE_INT);
    ASSERT_HAS(code, "STRI2INT GF@tmp4 GF@tmp0 GF@tmp1\n"); // valid instruction of the spec
    free(code);
}

static void test_builtin_ord_out_of_range_returns_minus_one(void **state) {
    (void)state;
    // spec 4: ord returns -1 for an empty string or index out of range (no runtime error)
    ast_node_t *args = string_node("abc");
    args->next = int_node(10);
    char *code = gen_builtin("ord", args, 2, SYM_DATATYPE_INT);
    ASSERT_HAS(code, "int@-1");
    free(code);
}

static void test_builtin_substr_uses_spec_name_and_valid_instructions(void **state) {
    (void)state;
    ast_node_t *args = string_node("abcd");
    args->next = int_node(1);
    args->next->next = int_node(3);
    char *code = gen_builtin("subStr", args, 3, SYM_DATATYPE_STRING);
    // function in the spec is called subStr (case sensitive)
    ASSERT_HAS_NOT(code, "CALL subStr");
    assert_only_valid_opcodes(code);
    free(code);
}

static void test_builtin_substr_runtime_error_exit_40(void **state) {
    (void)state;
    ast_node_t *args = string_node("abcd");
    args->next = int_node(1);
    args->next->next = int_node(3);
    char *code = gen_builtin("subStr", args, 3, SYM_DATATYPE_STRING);
    ASSERT_HAS(code, "JUMPIFEQ $EXIT40 GF@tmp3 bool@true");
    free(code);
}

static void test_all_emitted_opcodes_are_valid_ifjcode26(void **state) {
    (void)state;
    // A bigger program touching most node types
    ast_node_t *d = ast_var_declaration_create(
        strdup("x"), 0, SYM_DATATYPE_INT, true,
        binop(AST_EXPRESSION_OPERATOR_ADD, int_node(1), int_node(2), SYM_DATATYPE_INT), 1);
    ast_node_t *a =
        ast_assignment_create(strdup("x"), 0,
                              binop(AST_EXPRESSION_OPERATOR_MUL, var_use("x", 0, SYM_DATATYPE_INT),
                                    int_node(2), SYM_DATATYPE_INT),
                              1);
    ast_node_t *w =
        ast_while_create(binop(AST_EXPRESSION_OPERATOR_SMALLER_EQUAL,
                               var_use("x", 0, SYM_DATATYPE_INT), int_node(100), SYM_DATATYPE_INT),
                         block_of(print_call(var_use("x", 0, SYM_DATATYPE_INT)), NULL), 1);
    ast_node_t *body = block_of(d, a);
    ast_block_append(body, w);
    ast_node_t *root = root_with(fun_def("f", NULL, body));
    bool ok = false;
    char *code = generate(root, &ok);
    assert_true(ok);
    assert_only_valid_opcodes(code);
    free(code);
    ast_dispose(&root);
}

int main(void) {
    const struct CMUnitTest tests[] = {
        T(test_header_is_first_line),
        T(test_skeleton_defines_temporaries_and_jumps_to_main),
        T(test_jump_to_main_comes_before_function_labels),
        T(test_codegen_to_unwritable_path_fails),
        T(test_main_function_body_is_generated),
        T(test_main_ends_with_exit_zero_before_exit40_label),
        T(test_int_constants),
        T(test_double_constant_uses_hex_float_format),
        T(test_string_constant),
        T(test_string_constant_escapes_space_and_special_characters),
        T(test_empty_string_constant),
        T(test_unit_constant_and_print_of_unit),
        T(test_variable_declaration),
        T(test_assignment),
        T(test_variable_use),
        T(test_main_frame_exists_for_local_variables),
        T(test_int_addition_with_overflow_check),
        T(test_double_addition_has_no_overflow_check),
        T(test_overflow_routine_not_emitted_when_unused),
        T(test_overflow_routine_emitted_once),
        T(test_subtraction_and_multiplication),
        T(test_int_division_uses_idivs_and_double_uses_divs),
        T(test_string_concatenation),
        T(test_concatenation_keeps_operand_order),
        T(test_relational_operators),
        T(test_less_equal_keeps_operand_order),
        T(test_greater_equal_keeps_operand_order),
        T(test_unary_minus_is_generated),
        T(test_unary_plus_is_generated),
        T(test_int_to_double_conversion_node),
        T(test_if_else_labels),
        T(test_two_ifs_have_unique_labels),
        T(test_nested_ifs_have_unique_labels),
        T(test_while_loop),
        T(test_while_condition_is_checked_before_the_first_iteration),
        T(test_while_as_last_statement_does_not_crash),
        T(test_variable_in_loop_body_is_defined_once),
        T(test_function_definition_prologue),
        T(test_function_parameters_are_popped_in_reverse_order),
        T(test_return_pops_frame_and_returns),
        T(test_function_without_explicit_return_still_returns),
        T(test_user_function_call),
        T(test_function_label_cannot_clash_with_internal_labels),
        T(test_builtin_read_functions),
        T(test_builtin_read_int_returns_zero_on_error),
        T(test_builtin_read_line_at_eof_returns_empty_string),
        T(test_builtin_to_str),
        T(test_builtin_to_int),
        T(test_builtin_str_length),
        T(test_builtin_chr),
        T(test_builtin_ord_uses_valid_instruction),
        T(test_builtin_ord_out_of_range_returns_minus_one),
        T(test_builtin_substr_uses_spec_name_and_valid_instructions),
        T(test_builtin_substr_runtime_error_exit_40),
        T(test_all_emitted_opcodes_are_valid_ifjcode26),
    };
    return cmocka_run_group_tests_name("codegen", tests, NULL, NULL);
}
