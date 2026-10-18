/*
-------------------------------------------------------
Name of project: Translator IFJ26
Authors: Zobal, Tajovský, Ondruška, Svoboda
Creation date: 11.10.2026
File: scanner_bbox_tests.c

Description: Black-box scanner tests.

-------------------------------------------------------
*/

#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>

#include <cmocka.h> // must be under std includes

#include "scanner.h"

// Test scanner initialization struct
void scanner_init_test(void **state) {
    (void)state; // Unused argument

    scanner_conf_t scanner_conf;
    scanner_init(&scanner_conf, stdin);

    // FILE *source_file
    assert_ptr_equal(scanner_conf.source_file, stdin);
    // int unread_chars_stack[UNREAD_CHARS_SIZE]
    for (size_t i = 0; i < UNREAD_CHARS_SIZE; i++) {
        assert_int_equal(scanner_conf.unread_chars_stack[i], 0);
    }
    // int unread_top
    assert_int_equal(scanner_conf.unread_top, 0);
    // int indent_stack[INDENT_STACK_SIZE]
    for (size_t i = 0; i < INDENT_STACK_SIZE; i++) {
        assert_int_equal(scanner_conf.indent_stack[i], 0);
    }
    // int indent_top
    assert_int_equal(scanner_conf.indent_top, 0);
}
