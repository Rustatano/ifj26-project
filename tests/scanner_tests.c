/*
-------------------------------------------------------
Name of project: Translator IFJ26
Authors: Zobal, Tajovský, Ondruška, Svoboda
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

// Test scanner initialization struct
void scanner_init_test(void **state) {
    (void)state; // Unused argument

    scanner_conf_t scanner_conf;
    scanner_init(&scanner_conf, stdin);

    assert_ptr_equal(scanner_conf.source_file, stdin);
    assert_int_equal(scanner_conf.unread_top, 0);
    assert_int_equal(scanner_conf.indent_top, 0);
}
