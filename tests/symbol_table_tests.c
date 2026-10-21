/*
-------------------------------------------------------
Name of project: Translator IFJ26
Authors: Zobal, Tajovský, Ondruška, Svoboda
Creation date: 20.10.2026
File: scanner_bbox_tests.c

Description: Unit tests for symbol table.

-------------------------------------------------------
*/

#include <setjmp.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <cmocka.h> // must be under std includes

#include "symbol_table.h"

// SECTION: symtable_init
void symtable_init_test(void **state) {
    // Init
    symtable_stack_t symtable_stack;
    bool res = symtable_init(&symtable_stack);

    // Assert
    assert_true(res);
    assert_null(symtable_stack.top);
    assert_null(symtable_stack.global);
}

// SECTION: symtable_create_avl
void symtable_create_avl_test(void **state) {
    // Init
    symtable_stack_t symtable_stack;
    bool res = symtable_init(&symtable_stack);

    symtable_create_avl(&symtable_stack);

    // Assert
    assert_non_null(symtable_stack.top);
    assert_non_null(symtable_stack.global);
}

// SECTION: symtable_pop_avl
void symtable_pop_avl_test(void **state) {}

// SECTION: symtable_insert_item
void symtable_insert_item_test(void **state) {}

// SECTION: symtable_search
void symtable_search_test(void **state) {}

// SECTION: symtable_stack_search_current
void symtable_stack_search_current_test(void **state) {}

// SECTION: symtable_stack_delete
void symtable_stack_delete_test(void **state) {}
