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

void symtable_init_null_test(void **state) {
    // Init
    bool res = symtable_init(NULL);

    // Assert
    assert_false(res);
}

// SECTION: symtable_create_avl
void symtable_create_avl_first_call_test(void **state) {
    // Init
    symtable_stack_t symtable_stack;
    symtable_init(&symtable_stack);

    bool res = symtable_create_avl(&symtable_stack);

    // Assert
    assert_true(res);
    assert_non_null(symtable_stack.top);
    assert_non_null(symtable_stack.global);
    assert_int_equal(sizeof(symtable_stack.top), sizeof(symtable_stack_item_t));
    assert_null(symtable_stack.top->tree_root);
    assert_null(symtable_stack.top->next);
    assert_ptr_equal(symtable_stack.top, symtable_stack.global);
}

void symtable_create_avl_second_call_test(void **state) {
    // Init
    symtable_stack_t symtable_stack;
    symtable_init(&symtable_stack);

    if (!symtable_create_avl(&symtable_stack)) {
        return;
    }

    symtable_stack_item_t *first_call_top = symtable_stack.top;

    bool res = symtable_create_avl(&symtable_stack);

    // Assert
    assert_true(res);
    assert_non_null(symtable_stack.top);
    assert_non_null(symtable_stack.global);
    assert_ptr_not_equal(symtable_stack.top, symtable_stack.global);
    assert_ptr_equal(symtable_stack.global, first_call_top);
    assert_null(symtable_stack.top->tree_root);
    assert_ptr_equal(symtable_stack.top->next, first_call_top);
}

void symtable_create_avl_null_test(void **state) {
    // Init
    bool res = symtable_create_avl(NULL);

    // Assert
    assert_false(res);
}

// SECTION: symtable_pop_avl
void symtable_pop_avl_test(void **state) {
    // Init
    symtable_stack_t symtable_stack;
    symtable_init(&symtable_stack);

    if (!symtable_create_avl(&symtable_stack)) {
        return;
    }

    symtable_stack_item_t *first_call_top = symtable_stack.top;

    bool res = symtable_create_avl(&symtable_stack);
}

// SECTION: symtable_insert_item
void symtable_insert_item_test(void **state) {}

// SECTION: symtable_search
void symtable_search_test(void **state) {}

// SECTION: symtable_stack_search_current
void symtable_stack_search_current_test(void **state) {}

// SECTION: symtable_stack_delete
void symtable_stack_delete_test(void **state) {}
