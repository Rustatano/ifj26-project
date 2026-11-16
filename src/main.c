/*
-------------------------------------------------------
Name of project: Translator IFJ26
Authors: Zobal, Tajovský, Ondruška, Svoboda
Creation date: 01.10.2026
File: main.c

Description: Main entry point for the IFJ26 language translator

-------------------------------------------------------
*/

#include "ast.h"
#include "codegen.h"
#include "parser.h"
#include "scanner.h"
#include "semantic.h"
#include "symtable.h"

#define EXIT_INTERNAL_ERROR 99

int main(void) {
    // Creating Scanner configuration struct
    scanner_conf_t scanner_conf;

    // Initialization of that struct with starting values
    scanner_init(&scanner_conf, stdin);

    // Initialize AST
    ast_node_t *ast_ptr = NULL;

    // Calling parser function with scanner configuration
    parse_result_t parse_res = parse(&ast_ptr, &scanner_conf);

    if (!parse_res.success) {
        int code =
            parse_res.error_code == PARSE_ERR_INTERNAL ? EXIT_INTERNAL_ERROR : parse_res.error_code;
        fprintf(stderr, "Syntactic analysis Error %d: %s on line %d\n", code, parse_res.error_msg,
                parse_res.error_line);
        ast_dispose(&ast_ptr);
        return code;
    }

    // Execute semantic analysis
    semantic_result_t semantic_res = semantic_analysis(ast_ptr);

    if (!semantic_res.success) {
        fprintf(stderr, "Semantic analysis Error %d: %s on line %d\n", semantic_res.error_code,
                semantic_res.error_msg, semantic_res.error_line);
        ast_dispose(&ast_ptr);
        return semantic_res.error_code;
    }

    // Execute target code generation
    bool codegen_res = codegen(ast_ptr);

    if (!codegen_res) {
        return EXIT_INTERNAL_ERROR;
    }

    return 0;
}
