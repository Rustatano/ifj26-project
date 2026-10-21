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

// Scanner test functions
extern void scanner_init_test(void **state);

int main(void) {
    const struct CMUnitTest symtable_tests[] = {
        cmocka_unit_test(symtable_init_null_test),
        cmocka_unit_test(symtable_init_test),
        cmocka_unit_test(symtable_create_avl_null_test),
        cmocka_unit_test(symtable_create_avl_first_call_test),
        cmocka_unit_test(symtable_create_avl_second_call_test),
    };
    const struct CMUnitTest scanner_tests[] = {
        cmocka_unit_test(scanner_init_test),
    };
    /*const struct CMUnitTest parser_tests[] = {};
    const struct CMUnitTest semantic_analyzer_tests[] = {};
    const struct CMUnitTest generator_tests[] = {};
    const struct CMUnitTest optimizer_tests[] = {};
    const struct CMUnitTest integration_tests[] = {};*/

    int res1 = cmocka_run_group_tests(symtable_tests, NULL, NULL);
    int res2 = cmocka_run_group_tests(scanner_tests, NULL, NULL);
    /*
    int res3 = cmocka_run_group_tests(parser_tests, NULL, NULL);
    int res4 = cmocka_run_group_tests(semantic_analyzer_tests, NULL, NULL);
    int res5 = cmocka_run_group_tests(generator_tests, NULL, NULL);
    int res6 = cmocka_run_group_tests(optimizer_tests, NULL, NULL);
    int res7 = cmocka_run_group_tests(integration_tests, NULL, NULL);
    */

    return res1 || res2 /*|| res3 || res4 || res5 || res6 || res7*/;
}
