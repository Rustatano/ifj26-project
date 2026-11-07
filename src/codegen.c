/*
-------------------------------------------------------
Name of project: Translator IFJ26
Authors: xtajovm00 (Matej Tajovsky)
Creation date: 02.11.2026
File: codegen.c

Description: Generates IFJcode26 from the Abstract Syntax Tree

-------------------------------------------------------
*/

#include "codegen.h"

// create file
FILE *create_file(const char *output_file) {
    FILE *file = fopen(output_file, "w");
    return file;
}

// main function for code generation
bool codegen(ast_node_t *ast_root, const char *output_file) {
    FILE *file = create_file(output_file);
    if (file == NULL) {
        fprintf(stderr, "Failed to create output file '%s'\n", output_file);
        return false;
    }
}
