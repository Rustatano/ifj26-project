/*
-------------------------------------------------------
Name of project: Translator IFJ26
Authors: Zobal, Tajovský, Ondruška, Svoboda
Creation date: 11.10.2026
File: run_all_tests.c

Description: Main file for running all tests.

-------------------------------------------------------
*/

#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>

#include <cmocka.h> // must be under std includes

// Symbol table test functions
extern void symtable_init_null_test(void **state);
extern void symtable_init_test(void **state);
extern void symtable_create_avl_null_test(void **state);
extern void symtable_create_avl_first_call_test(void **state);
extern void symtable_create_avl_second_call_test(void **state);
extern void symtable_pop_avl_null_test(void **state);
extern void symtable_pop_avl_one_tree_test(void **state);
extern void symtable_pop_avl_two_trees_test(void **state);
extern void symtable_pop_avl_empty_stack_test(void **state);
extern void symtable_insert_item_null_test(void **state);
extern void symtable_insert_item_success_test(void **state);
extern void symtable_search_not_found_test(void **state);
extern void symtable_search_found_test(void **state);
extern void symtable_search_global_found_test(void **state);
extern void symtable_search_shadowing_test(void **state);
extern void symtable_search_null_args_test(void **state);
extern void symtable_stack_search_current_null_test(void **state);
extern void symtable_stack_search_current_exists_test(void **state);
extern void symtable_stack_search_current_not_exists_test(void **state);
extern void symtable_stack_delete_test(void **state);

// Scanner test functions
extern void scanner_init_test(void **state);

int main(void) {
    // Define test groups
    const struct CMUnitTest symtable_tests[] = {
        cmocka_unit_test(symtable_init_null_test),
        cmocka_unit_test(symtable_init_test),
        cmocka_unit_test(symtable_create_avl_null_test),
        cmocka_unit_test(symtable_create_avl_first_call_test),
        cmocka_unit_test(symtable_create_avl_second_call_test),
        cmocka_unit_test(symtable_pop_avl_null_test),
        cmocka_unit_test(symtable_pop_avl_one_tree_test),
        cmocka_unit_test(symtable_pop_avl_two_trees_test),
        cmocka_unit_test(symtable_pop_avl_empty_stack_test),
        cmocka_unit_test(symtable_insert_item_null_test),
        cmocka_unit_test(symtable_insert_item_success_test),
        cmocka_unit_test(symtable_search_not_found_test),
        cmocka_unit_test(symtable_search_found_test),
        cmocka_unit_test(symtable_search_global_found_test),
        cmocka_unit_test(symtable_search_shadowing_test),
        cmocka_unit_test(symtable_search_null_args_test),
        cmocka_unit_test(symtable_stack_search_current_null_test),
        cmocka_unit_test(symtable_stack_search_current_exists_test),
        cmocka_unit_test(symtable_stack_search_current_not_exists_test),
        cmocka_unit_test(symtable_stack_delete_test),
    };
    const struct CMUnitTest scanner_tests[] = {
        cmocka_unit_test(scanner_init_test),
    };

    // Run test groups
    int res1 = cmocka_run_group_tests(symtable_tests, NULL, NULL);
    int res2 = cmocka_run_group_tests(scanner_tests, NULL, NULL);

    // Evaluate return value
    return res1 || res2;
}
