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

typedef struct {
    FILE *file;
    int label_counter;
    bool is_in_loop; // used for handling var declaration vs assignment
} codegen_context_t;

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
    context->is_in_loop = false;
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

bool handle_node(ast_node_t *node, codegen_context_t *context) {
    if (node == NULL) {
        return true;
    }

    switch (node->type) {
    case AST_NODE_PROGRAM:
        handle_node(node->block.first_statement, context);
        break;
    case AST_NODE_BLOCK:
        handle_node(node->block.first_statement, context);
        break;
    case AST_NODE_FUN_DEFINITION:
        break;
    case AST_NODE_VAR_DECLARATION:
        // define the variable if outside a loop
        if (!context->is_in_loop) {
            fprintf(context->file, "DEFVAR LF@%s$%s\n", node->var_declaration.name,
                    node->var_declaration.unique_id);
        }
        if (node->var_declaration.init_expres != NULL) {
            // handle the expression, result is on top of the stack
            handle_node(node->var_declaration.init_expres, context);
            // save the result from the stack to the variable
            fprintf(context->file, "POPS LF@%s$%s\n", node->var_declaration.name,
                    node->var_declaration.unique_id);
        }
        break;

    case AST_NODE_VALUE_ASSIGNMENT:
        // same as variable declaration but just the assignment part
        handle_node(node->assign_statement.expres, context);
        fprintf(context->file, "POPS LF@%s$%s\n", node->assign_statement.target_var_name,
                node->assign_statement.unique_id);
        break;

    case AST_NODE_STATEMENT_IF:
        break;
    case AST_NODE_STATEMENT_WHILE:
        context->is_in_loop = true;
        // extract variable declarations first in case this block is a loop block
        extract_var_declarations(node->next->block.first_statement, context);

        int label_id = context->label_counter;
        context->label_counter++;
        fprintf(context->file, "LABEL $WHILE$%d\n", label_id);

        handle_node(node->while_statement.body_block, context);

        // while condition
        handle_node(node->while_statement.condition, context);
        // result of the condition is on top of the stack
        // push true to the stack and check if the condition is true
        printfs(context->file, "PUSHS bool@true\n");
        printfs(context->file, "JUMPIFEQS $WHILE$%d\n", label_id);

        context->is_in_loop = false;
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
        fprintf(stderr, "Unknown AST node\n");
        return false;
    }

    handle_node(node->next, context);
    return true;
}
// this is used to extract variable declarations from block nodes
// so there is no multiple var declarations in case of a loop block
void extract_var_declarations(ast_node_t *node, codegen_context_t *context) {
    if (node == NULL) {
        return true;
    }

    if (node->type == AST_NODE_VAR_DECLARATION) {
        fprintf(context->file, "DEFVAR LF@%s$%s\n", node->var_declaration.name,
                node->var_declaration.unique_id);
    }

    extract_var_declarations(node->next, context);
}

// main function for code generation
bool codegen(ast_node_t *ast_root, const char *output_file) {
    // create context including the output file
    codegen_context_t *context = create_codegen_context(output_file);
    if (context == NULL) {
        return false;
    }

    bool result = handle_node(ast_root, context);

    destroy_codegen_context(context);
    return result;
}
