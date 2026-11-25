/*
-------------------------------------------------------
Name of project: Translator IFJ26
Authors: xsvoboj01 (Jakub Svoboda)
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
#include <stdio.h>
#include <stdlib.h>

#define EXIT_INTERNAL_ERROR 99

int main(void) {
    // Creating Scanner configuration struct
    scanner_conf_t scanner_conf;

    // Initialization of that struct with starting values
    scanner_init(&scanner_conf, stdin);

    // Initialize AST
    ast_node_t *ast_ptr = NULL;

    // Calling parser function with scanner configuration
    // (lexical errors are reported by the scanner itself and end the program with code 1)
    parse_result_t parse_res = parse(&ast_ptr, &scanner_conf);

    if (!parse_res.success) {
        int code =
            parse_res.error_code == PARSE_ERR_INTERNAL ? EXIT_INTERNAL_ERROR : parse_res.error_code;
        fprintf(stderr, "Syntactic analysis Error %d: %s on line %d\n", code,
                parse_res.error_msg != NULL ? parse_res.error_msg : "unknown error",
                parse_res.error_line);
        ast_dispose(&ast_ptr);
        return code;
    }

    // Initialize symbol table (semantic analysis creates the global scope)
    symtable_stack_t symtable_stack;

    // Execute semantic analysis
    semantic_result_t semantic_res = semantic_analysis(ast_ptr, &symtable_stack);

    if (!semantic_res.success) {
        fprintf(stderr, "Semantic analysis Error %d: %s on line %d\n", semantic_res.error_code,
                semantic_res.error_msg != NULL ? semantic_res.error_msg : "unknown error",
                semantic_res.error_line);
        ast_dispose(&ast_ptr);
        return semantic_res.error_code;
    }

    // Execute target code generation, the code is written to the standard output
    bool codegen_res = codegen(ast_ptr, NULL);

    symtable_stack_delete(&symtable_stack);
    ast_dispose(&ast_ptr);

    if (!codegen_res) {
        return EXIT_INTERNAL_ERROR;
    }

    return 0;
}
