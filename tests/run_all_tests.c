/*
-------------------------------------------------------
Name of project: Translator IFJ26
Authors: AI generated (Gemini), xsvoboj01 (Jakub Svoboda)
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
extern void get_token_eof_test(void **state);
extern void get_token_keyword_and_id_test(void **state);
extern void get_token_numbers_test(void **state);
extern void get_token_operators_test(void **state);
extern void get_token_string_literal_test(void **state);

// AST test functions
extern void ast_node_create_test(void **state);
extern void ast_const_int_create_test(void **state);
extern void ast_const_double_create_test(void **state);
extern void ast_const_string_create_test(void **state);
extern void ast_block_create_and_append_test(void **state);
extern void ast_expres_operation_create_test(void **state);
extern void ast_var_declaration_create_test(void **state);
extern void ast_assignment_create_test(void **state);
extern void ast_if_create_test(void **state);
extern void ast_dispose_null_test(void **state);

// Semantic test functions
extern void semantic_init_null_test(void **state);
extern void semantic_init_success_test(void **state);
extern void count_fun_params_null_test(void **state);
extern void count_fun_params_multiple_test(void **state);
extern void semantic_analysis_null_test(void **state);

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
        cmocka_unit_test(get_token_eof_test),
        cmocka_unit_test(get_token_keyword_and_id_test),
        cmocka_unit_test(get_token_numbers_test),
        cmocka_unit_test(get_token_operators_test),
        cmocka_unit_test(get_token_string_literal_test),
    };

    const struct CMUnitTest ast_tests[] = {
        cmocka_unit_test(ast_node_create_test),
        cmocka_unit_test(ast_const_int_create_test),
        cmocka_unit_test(ast_const_double_create_test),
        cmocka_unit_test(ast_const_string_create_test),
        cmocka_unit_test(ast_block_create_and_append_test),
        cmocka_unit_test(ast_expres_operation_create_test),
        cmocka_unit_test(ast_var_declaration_create_test),
        cmocka_unit_test(ast_assignment_create_test),
        cmocka_unit_test(ast_if_create_test),
        cmocka_unit_test(ast_dispose_null_test),
    };

    const struct CMUnitTest semantic_tests[] = {
        cmocka_unit_test(semantic_init_null_test),
        cmocka_unit_test(semantic_init_success_test),
        cmocka_unit_test(count_fun_params_null_test),
        cmocka_unit_test(count_fun_params_multiple_test),
        cmocka_unit_test(semantic_analysis_null_test),
    };

    // Run test groups
    int res1 = cmocka_run_group_tests(symtable_tests, NULL, NULL);
    int res2 = cmocka_run_group_tests(scanner_tests, NULL, NULL);
    int res3 = cmocka_run_group_tests(ast_tests, NULL, NULL);
    int res4 = cmocka_run_group_tests(semantic_tests, NULL, NULL);

    // Evaluate return value
    return res1 || res2 || res3 || res4;
}
