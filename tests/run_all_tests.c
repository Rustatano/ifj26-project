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

extern void scanner_init_test(void **state);

int main(void) {
    const struct CMUnitTest scanner_tests[] = {
        cmocka_unit_test(scanner_init_test),
    };
    /*const struct CMUnitTest parser_tests[] = {};
    const struct CMUnitTest semantic_analyzer_tests[] = {};
    const struct CMUnitTest generator_tests[] = {};
    const struct CMUnitTest optimizer_tests[] = {};
    const struct CMUnitTest integration_tests[] = {};*/

    return cmocka_run_group_tests(scanner_tests, NULL, NULL) /*&&
           cmocka_run_group_tests(parser_tests, NULL, NULL) &&
           cmocka_run_group_tests(semantic_analyzer_tests, NULL, NULL) &&
           cmocka_run_group_tests(generator_tests, NULL, NULL) &&
           cmocka_run_group_tests(optimizer_tests, NULL, NULL) &&
           cmocka_run_group_tests(integration_tests, NULL, NULL)*/
        ;
}
