/*
-------------------------------------------------------
Name of project: Translator IFJ26
Authors: AI generated (Gemini), xsvoboj01 (Jakub Svoboda)
Creation date: 11.10.2026
File: generator_tests.c

Description: Unit tests for generator of IFJcode26.

-------------------------------------------------------
*/

#include <setjmp.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#include <cmocka.h> // must be under std includes

#include "ast.h"
#include "codegen.h"

void codegen_context_create_destroy_test(void **state) {
    (void)state;
    // Init
    const char *test_filename = "test_output.code";
    codegen_context_t *ctx = create_codegen_context(test_filename);

    // Assert
    assert_non_null(ctx);
    assert_non_null(ctx->file);
    assert_int_equal(ctx->label_counter, 0);
    assert_false(ctx->is_in_loop);

    destroy_codegen_context(ctx);
    remove(test_filename);
}

void codegen_context_null_destroy_test(void **state) {
    (void)state;
    // Init
    codegen_context_t *ctx = NULL;

    // Assert
    destroy_codegen_context(ctx); // Should not crash
}

void codegen_check_function_is_builtin_test(void **state) {
    (void)state;
    // Init
    bool res1 = check_function_is_builtin("print");
    bool res2 = check_function_is_builtin("readInt");
    bool res3 = check_function_is_builtin("nonexistent_fun");

    // Assert
    assert_true(res1);
    assert_true(res2);
    assert_false(res3);
}

void codegen_run_null_ast_test(void **state) {
    (void)state;
    // Init
    const char *test_filename = "test_null_ast.code";
    bool res = codegen(NULL, test_filename);

    // Assert
    assert_true(res);
    remove(test_filename);
}
