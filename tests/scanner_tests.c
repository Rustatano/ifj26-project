/*
-------------------------------------------------------
Name of project: Translator IFJ26
Authors: AI generated (Gemini), xsvoboj01 (Jakub Svoboda)
Creation date: 11.10.2026
File: scanner_tests.c

Description: Unit tests for scanner.

-------------------------------------------------------
*/

#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>

#include <cmocka.h> // must be under std includes

#include "scanner.h"

void get_token_keyword_and_id_test(void **state) {
    (void)state; // Unused argument
    // Init
    scanner_conf_t conf;
    FILE *f = tmpfile();
    fputs("def foo", f);
    rewind(f);
    scanner_init(&conf, f);

    token_t token1 = get_token(&conf);
    token_t token2 = get_token(&conf);

    // Assert
    assert_int_equal(token1.type, TOKEN_KW_DEF);
    assert_int_equal(token2.type, TOKEN_ID);

    if (f) {
        fclose(f);
    }
}

void get_token_numbers_test(void **state) {
    (void)state; // Unused argument
    // Init
    scanner_conf_t conf;
    FILE *f = tmpfile();
    fputs("42 3.14", f);
    rewind(f);
    scanner_init(&conf, f);

    token_t token1 = get_token(&conf);
    token_t token2 = get_token(&conf);

    // Assert
    assert_int_equal(token1.type, TOKEN_INT);
    assert_int_equal(token1.attr.int_val, 42);
    assert_int_equal(token2.type, TOKEN_DOUBLE);
    assert_true(token2.attr.double_val == 3.14);

    if (f) {
        fclose(f);
    }
}

void get_token_operators_test(void **state) {
    (void)state; // Unused argument
    // Init
    scanner_conf_t conf;
    FILE *f = tmpfile();
    fputs("+ - == !=", f);
    rewind(f);
    scanner_init(&conf, f);

    token_t t1 = get_token(&conf);
    token_t t2 = get_token(&conf);
    token_t t3 = get_token(&conf);
    token_t t4 = get_token(&conf);

    // Assert
    assert_int_equal(t1.type, TOKEN_PLUS);
    assert_int_equal(t2.type, TOKEN_MINUS);
    assert_int_equal(t3.type, TOKEN_EQ);
    assert_int_equal(t4.type, TOKEN_NEQ);

    if (f) {
        fclose(f);
    }
}

void get_token_string_literal_test(void **state) {
    (void)state; // Unused argument
    // Init
    scanner_conf_t conf;
    FILE *f = tmpfile();
    fputs("\"hello world\"", f);
    rewind(f);
    scanner_init(&conf, f);

    token_t token = get_token(&conf);

    // Assert
    assert_int_equal(token.type, TOKEN_STRING);
    assert_string_equal(token.attr.str_val, "hello world");

    if (f) {
        fclose(f);
    }
}
