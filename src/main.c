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
        fprintf(stderr, "Syntactic analysis Error %d: %s on line %d\n", parse_res.error_code,
                parse_res.error_msg, parse_res.error_line);
        exit(parse_res.error_code);
    }

    // Initialize symbol table
    symtable_stack_t symtable_stack;
    symtable_init(&symtable_stack);

    // Execute semantic analysis
    semantic_result_t semantic_res = semantic_analysis(ast_ptr, &symtable_stack);

    if (!semantic_res.success) {
        fprintf(stderr, "Semantic analysis Error %d: %s on line %d\n", semantic_res.error_code,
                semantic_res.error_msg, semantic_res.error_line);
        exit(semantic_res.error_code);
    }

    // Execute target code generation
    bool codegen_res = codegen(ast_ptr, "OUTPUT.ifjcode");

    if (!codegen_res) {
        exit(1); // TODO correct code
    }

    return 0;
}
