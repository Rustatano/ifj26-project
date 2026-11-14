/*
-------------------------------------------------------
Name of project: Translator IFJ26
Authors: Zobal, Tajovský, Ondruška, Svoboda
Creation date: 11.10.2026
File: generator_tests.c

Description: Unit tests for generator of IFJcode26.

-------------------------------------------------------
*/

#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <cmocka.h> // must be under std includes

#include "codegen.h"

#define CODEGEN_TEST_OUTPUT "/tmp/ifj26_codegen_test.out"

static ast_node_t *create_test_node(ast_node_type_t type) {
    ast_node_t *node = calloc(1, sizeof(*node));
    assert_non_null(node);
    node->type = type;
    node->source_code_line = 1;
    return node;
}

static ast_node_t *create_test_main(void) { return create_test_node(AST_NODE_MAIN); }

static ast_node_t *create_test_int(int value) {
    ast_node_t *node = create_test_node(AST_NODE_CONST_INT);
    node->int_value = value;
    node->result_type = SYM_DATATYPE_INT;
    return node;
}

static void free_test_ast(ast_node_t *node) {
    if (node == NULL) {
        return;
    }

    free_test_ast(node->next);
    switch (node->type) {
    case AST_NODE_VAR_DECLARATION:
        free_test_ast(node->var_declaration.init_expres);
        break;
    case AST_NODE_VALUE_ASSIGNMENT:
        free_test_ast(node->assign_statement.expres);
        break;
    case AST_NODE_EXPRES_OPERATOR:
        free_test_ast(node->expres_operator.left_side);
        free_test_ast(node->expres_operator.right_side);
        break;
    default:
        break;
    }
    free(node);
}

static char *read_test_output(void) {
    FILE *file = fopen(CODEGEN_TEST_OUTPUT, "r");
    assert_non_null(file);
    assert_int_equal(fseek(file, 0, SEEK_END), 0);
    long file_size = ftell(file);
    assert_true(file_size >= 0);
    rewind(file);

    char *output = calloc((size_t)file_size + 1, sizeof(*output));
    assert_non_null(output);
    assert_int_equal(fread(output, 1, (size_t)file_size, file), (size_t)file_size);
    assert_int_equal(fclose(file), 0);
    return output;
}

void codegen_program_and_constants_test(void **state) {
    (void)state;
    ast_node_t *main_node = create_test_main();
    ast_node_t *int_node = create_test_int(42);
    ast_node_t *double_node = create_test_node(AST_NODE_CONST_DOUBLE);
    double_node->double_value = 1.5;
    double_node->result_type = SYM_DATATYPE_DOUBLE;
    ast_node_t *string_node = create_test_node(AST_NODE_CONST_STRING);
    string_node->str_value = "hello";
    string_node->result_type = SYM_DATATYPE_STRING;
    main_node->next = int_node;
    int_node->next = double_node;
    double_node->next = string_node;

    bool result = codegen(main_node, CODEGEN_TEST_OUTPUT);
    char *output = read_test_output();

    assert_true(result);
    assert_non_null(strstr(output, ".IFJcode26\n"));
    assert_non_null(strstr(output, "LABEL main\n"));
    assert_non_null(strstr(output, "PUSHS int@42\n"));
    assert_non_null(strstr(output, "PUSHS float@0x1.8p+0\n"));
    assert_non_null(strstr(output, "PUSHS string@hello\n"));
    assert_non_null(strstr(output, "LABEL $RETURN40\n"));

    free(output);
    free_test_ast(main_node);
    remove(CODEGEN_TEST_OUTPUT);
}

void codegen_variable_declaration_and_assignment_test(void **state) {
    (void)state;
    ast_node_t *main_node = create_test_main();
    ast_node_t *declaration = create_test_node(AST_NODE_VAR_DECLARATION);
    declaration->var_declaration.name = "value";
    declaration->var_declaration.unique_id = 7;
    declaration->var_declaration.var_type = SYM_DATATYPE_INT;
    declaration->var_declaration.init_expres = create_test_int(10);
    ast_node_t *assignment = create_test_node(AST_NODE_VALUE_ASSIGNMENT);
    assignment->assign_statement.target_var_name = "value";
    assignment->assign_statement.unique_id = 7;
    assignment->assign_statement.expres = create_test_int(20);
    main_node->next = declaration;
    declaration->next = assignment;

    bool result = codegen(main_node, CODEGEN_TEST_OUTPUT);
    char *output = read_test_output();

    assert_true(result);
    assert_non_null(strstr(output, "DEFVAR LF@value$7\n"));
    assert_non_null(strstr(output, "PUSHS int@10\nPOPS LF@value$7\n"));
    assert_non_null(strstr(output, "PUSHS int@20\nPOPS LF@value$7\n"));

    free(output);
    free_test_ast(main_node);
    remove(CODEGEN_TEST_OUTPUT);
}

void codegen_integer_arithmetic_test(void **state) {
    (void)state;
    ast_node_t *main_node = create_test_main();
    ast_node_t *operation = create_test_node(AST_NODE_EXPRES_OPERATOR);
    operation->expres_operator.op = AST_EXPRES_OPERATOR_ADD;
    operation->expres_operator.left_side = create_test_int(1);
    operation->expres_operator.right_side = create_test_int(2);
    operation->result_type = SYM_DATATYPE_INT;
    main_node->next = operation;

    bool result = codegen(main_node, CODEGEN_TEST_OUTPUT);
    char *output = read_test_output();

    assert_true(result);
    assert_non_null(strstr(output, "PUSHS int@1\nPUSHS int@2\nADDS\nCALL $OVERFLOW_CHECK\n"));
    assert_non_null(strstr(output, "LABEL $OVERFLOW_CHECK\n"));
    assert_non_null(strstr(output, "LABEL $OVERFLOW_POSITIVE\n"));
    assert_non_null(strstr(output, "LABEL $OVERFLOW_NEGATIVE\n"));

    free(output);
    free_test_ast(main_node);
    remove(CODEGEN_TEST_OUTPUT);
}
