/*
-------------------------------------------------------
Name of project: Translator IFJ26
Authors:AI generated (Gemini), xsvoboj01 (Jakub Svoboda)
Creation date: 11.10.2026
File: semantic_analyzer_tests.c

Description: Unit tests for semantic analyzer.

-------------------------------------------------------
*/

#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>

#include <cmocka.h> // must be under std includes

#include "ast.h"
#include "semantic.h"
#include "symbol_table.h"

void semantic_init_null_test(void **state) {
    (void)state;
    // Init
    bool res = semantic_init(NULL);

    // Assert
    assert_false(res);
}

void semantic_init_success_test(void **state) {
    (void)state;
    // Init
    symtable_stack_t stack;
    bool res = semantic_init(&stack);

    // Assert
    assert_true(res);
    assert_non_null(stack.global);
    assert_ptr_equal(stack.top, stack.global);

    symtable_stack_delete(&stack);
}

void count_fun_params_null_test(void **state) {
    (void)state;
    // Init
    int count = count_fun_params(NULL);

    // Assert
    assert_int_equal(count, 0);
}

void count_fun_params_multiple_test(void **state) {
    (void)state;
    // Init
    symtable_fun_param_t p3 = {.name = "c", .type = SYM_DATATYPE_STRING, .next = NULL};
    symtable_fun_param_t p2 = {.name = "b", .type = SYM_DATATYPE_DOUBLE, .next = &p3};
    symtable_fun_param_t p1 = {.name = "a", .type = SYM_DATATYPE_INT, .next = &p2};

    int count = count_fun_params(&p1);

    // Assert
    assert_int_equal(count, 3);
}

void semantic_analysis_null_test(void **state) {
    (void)state;
    // Init
    symtable_stack_t stack;
    symtable_init(&stack);
    ast_node_t *ast = NULL;

    bool res1 = semantic_analysis(NULL, &stack);
    bool res2 = semantic_analysis(ast, NULL);

    // Assert
    assert_false(res1);
    assert_false(res2);

    symtable_stack_delete(&stack);
}
