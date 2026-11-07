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

codegen_context_t *create_codegen_context(const char *output_file) {
    FILE *file = create_file(output_file);
    if (file == NULL) {
        fprintf(stderr, "Failed to create file\n");
        return NULL;
    }

    codegen_context_t *context = malloc(sizeof(codegen_context_t));
    if (context == NULL) {
        fprintf(stderr, "Failed malloc\n");
        return NULL;
    }
    context->file = file;
    context->label_counter = 0;
    return context;
}

void destroy_codegen_context(codegen_context_t *context) {
    if (context != NULL) {
        if (context->file != NULL) {
            fclose(context->file);
        }
        free(context);
    }
}

void handle_node(ast_node_t *node, codegen_context_t *context) {
    if (node == NULL) {
        return;
    }

    switch (node->type) {
    case AST_NODE_PROGRAM:
        break;
    case AST_NODE_BLOCK:
        break;
    case AST_NODE_FUN_DEFINITION:
        break;
    case AST_NODE_VAR_DECLARATION:
        break;
    case AST_NODE_VALUE_ASSIGNMENT:
        break;
    case AST_NODE_STATEMENT_IF:
        break;
    case AST_NODE_STATEMENT_WHILE:
        break;
    case AST_NODE_STATEMENT_RETURN:
        break;
    case AST_NODE_STATEMENT_IMPORT:
        break;
    case AST_NODE_EXPRES_OPERATOR:
        break;
    case AST_NODE_EXPRES_FUN_CALL:
        break;
    case AST_NODE_EXPRES_VAR:
        break;
    case AST_NODE_CONST_INT:
        break;
    case AST_NODE_CONST_DOUBLE:
        break;
    case AST_NODE_CONST_STRING:
        break;
    case AST_NODE_CONST_UNIT:
        break;
    default:
        fprintf(stderr, "Unknown AST node type: %d\n", node->type);
    }
}

// main function for code generation
bool codegen(ast_node_t *ast_root, const char *output_file) {
    // create context including the output file
    codegen_context_t *context = create_codegen_context(output_file);
    if (context == NULL) {
        return false;
    }

    handle_node(ast_root, context);

    destroy_codegen_context(context);
    return true;
}
