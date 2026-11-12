/*
-------------------------------------------------------
Name of project: Translator IFJ26
Authors: Zobal, Tajovský, Ondruška, Svoboda
Creation date: 11.10.2026
File: parser_tests.c

Description: Unit tests for parser.

-------------------------------------------------------
*/

#include <setjmp.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#include <cmocka.h> // must be under std includes

#include "ast.h"
#include "parser.h"
#include "scanner.h"

void parser_parse_null_test(void **state) {
    (void)state;
    // Init
    ast_node_t *ast_root = NULL;
    scanner_conf_t conf;

    // Assert
    parse_result_t res = parse(&ast_root, &conf);
    assert_false(res.success);
    assert_null(ast_root);
}

void parser_remap_operator_test(void **state) {
    (void)state;
    // Init
    ast_expres_operator_t op1 = remap_operator(TOKEN_PLUS);
    ast_expres_operator_t op2 = remap_operator(TOKEN_MINUS);
    ast_expres_operator_t op3 = remap_operator(TOKEN_MUL);
    ast_expres_operator_t op4 = remap_operator(TOKEN_DIV);

    // Assert
    assert_int_equal(op1, AST_EXPRES_OPERATOR_ADD);
    assert_int_equal(op2, AST_EXPRES_OPERATOR_SUB);
    assert_int_equal(op3, AST_EXPRES_OPERATOR_MUL);
    assert_int_equal(op4, AST_EXPRES_OPERATOR_DIV);
}

void parser_parse_data_type_test(void **state) {
    (void)state;
    // Init
    scanner_conf_t conf;
    FILE *f = tmpfile();
    fputs("Int Double String Unit Unknown", f);
    rewind(f);
    scanner_init(&conf, f);

    parser_context_t ctx = {.scanner_conf = &conf, .current_token = get_token(&conf)};

    // Assert
    assert_int_equal(parse_data_type(&ctx), SYM_DATATYPE_INT);
    load_token(&ctx);
    assert_int_equal(parse_data_type(&ctx), SYM_DATATYPE_DOUBLE);
    load_token(&ctx);
    assert_int_equal(parse_data_type(&ctx), SYM_DATATYPE_STRING);
    load_token(&ctx);
    assert_int_equal(parse_data_type(&ctx), SYM_DATATYPE_UNIT);
    load_token(&ctx);
    assert_int_equal(parse_data_type(&ctx), SYM_DATATYPE_UNKNOWN);

    if (f) {
        fclose(f);
    }
}
