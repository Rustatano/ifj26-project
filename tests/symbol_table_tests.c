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
void symtable_init_null_test(void **state) {
    (void)state; // Unused argument
    // Init
    bool res = symtable_init(NULL);

    // Assert
    assert_false(res);
}

void symtable_init_test(void **state) {
    (void)state; // Unused argument
    // Init
    symtable_stack_t symtable_stack;
    bool res = symtable_init(&symtable_stack);

    // Assert
    assert_true(res);
    assert_null(symtable_stack.top);
    assert_null(symtable_stack.global);
}

// SECTION: symtable_create_avl
void symtable_create_avl_null_test(void **state) {
    (void)state; // Unused argument
    // Init
    bool res = symtable_create_avl(NULL);

    // Assert
    assert_false(res);
}

void symtable_create_avl_first_call_test(void **state) {
    (void)state; // Unused argument
    // Init
    symtable_stack_t symtable_stack;
    symtable_init(&symtable_stack);

    bool res = symtable_create_avl(&symtable_stack);

    // Assert
    assert_true(res);
    assert_non_null(symtable_stack.top);
    assert_non_null(symtable_stack.global);
    assert_null(symtable_stack.top->tree_root);
    assert_null(symtable_stack.top->next);
    assert_ptr_equal(symtable_stack.top, symtable_stack.global);
}

void symtable_create_avl_second_call_test(void **state) {
    (void)state; // Unused argument
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

// SECTION: symtable_pop_avl
void symtable_pop_avl_null_test(void **state) {
    (void)state; // Unused argument
    // Init
    symtable_pop_avl(NULL);

    // Assert
    // assert(not crashing)
}

void symtable_pop_avl_one_tree_test(void **state) {
    (void)state; // Unused argument
    // Init
    symtable_stack_t symtable_stack;
    symtable_init(&symtable_stack);

    if (!symtable_create_avl(&symtable_stack)) {
        return;
    }

    symtable_pop_avl(&symtable_stack);

    // Assert
    assert_null(symtable_stack.global);
    assert_null(symtable_stack.top);
}

void symtable_pop_avl_two_trees_test(void **state) {
    (void)state; // Unused argument
    // Init
    symtable_stack_t symtable_stack;
    symtable_init(&symtable_stack);

    if (!symtable_create_avl(&symtable_stack)) {
        return;
    }

    symtable_stack_item_t *prev_global = symtable_stack.global;
    symtable_stack_item_t *prev_top = symtable_stack.top;

    if (!symtable_create_avl(&symtable_stack)) {
        return;
    }

    // Assert
    assert_ptr_equal(prev_global, symtable_stack.global);
    assert_ptr_not_equal(prev_top, symtable_stack.top);
    assert_non_null(symtable_stack.top);
}

// SECTION: symtable_insert_item
void symtable_insert_item_test(void **state) {
    (void)state; // Unused argument
}

// SECTION: symtable_search
void symtable_search_test(void **state) {
    (void)state; // Unused argument
    // Init

    // Assert
}

// SECTION: symtable_stack_search_current
void symtable_stack_search_current_null_test(void **state) {
    (void)state; // Unused argument
    // Init
    symtable_node_t *res = symtable_stack_search_current(NULL, SYM_TYPE_VAR, NULL);

    // Assert
    assert_null(res);
}

void symtable_stack_search_current_exists_test(void **state) {
    (void)state; // Unused argument
    // Init
    symtable_stack_t symtable_stack;
    symtable_init(&symtable_stack);

    if (!symtable_create_avl(&symtable_stack)) {
        return;
    }

    symtable_data_t data;
    data.type = SYM_TYPE_VAR;
    data.var.type = SYM_TYPE_ID;
    data.var.is_mutable = false;
    data.var.is_initialized = true;
    data.var.unique_id = 19;

    symtable_insert_item(&symtable_stack, "ID_X", data);

    symtable_node_t *res = symtable_stack_search_current(symtable_stack.top, SYM_TYPE_VAR, "ID_X");

    // Assert
    assert_non_null(res);
    assert_int_equal(res->data.type, SYM_TYPE_VAR);
    assert_int_equal(res->data.var.type, SYM_TYPE_ID);
    assert_false(res->data.var.is_mutable);
    assert_true(res->data.var.is_initialized);
    assert_int_equal(res->data.var.unique_id, 19);
}

void symtable_stack_search_current_not_exists_test(void **state) {
    (void)state; // Unused argument
    // Init
    symtable_stack_t symtable_stack;
    symtable_init(&symtable_stack);

    if (!symtable_create_avl(&symtable_stack)) {
        return;
    }

    symtable_data_t data;
    data.type = SYM_TYPE_VAR;
    data.var.type = SYM_TYPE_ID;
    data.var.is_mutable = false;
    data.var.is_initialized = true;
    data.var.unique_id = 19;

    symtable_insert_item(&symtable_stack, "ID_X", data);

    symtable_node_t *res1 =
        symtable_stack_search_current(symtable_stack.top, SYM_TYPE_FUNC, "ID_X");
    symtable_node_t *res2 = symtable_stack_search_current(symtable_stack.top, SYM_TYPE_VAR, "ID_Y");

    // Assert
    assert_null(res1);
    assert_null(res2);
}

// SECTION: symtable_stack_delete
void symtable_stack_delete_test(void **state) {
    (void)state; // Unused argument
}
