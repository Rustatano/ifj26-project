/*
-------------------------------------------------------
Name of project: Translator IFJ26
Authors: Zobal, Tajovský, Ondruška, Svoboda
Creation date: 18.10.2026
File: semantic.c

Description: Implementation of semantic analyzer

-------------------------------------------------------
*/

#include "semantic.h"

bool semantic_init(symtable_stack_t *symtable_stack) {
    if (symtable_stack == NULL) {
        return false;
    }

    // Initialize symtable stack
    if (!symtable_init(symtable_stack)) {
        return false;
    }

    // Create global symtable on top of stack
    if (!symtable_create_avl(symtable_stack)) {
        return false;
    }

    return true;
}

// Count function parameters recursively
int count_fun_params(symtable_fun_param_t *param) {
    if (param == NULL) {
        return 0;
    }

    return count_fun_params(param->next) + 1;
}

bool first_ast_passage(ast_node_t *ast, symtable_stack_t *symtable_stack) {
    if (ast == NULL || symtable_stack == NULL) {
        return false;
    }

    ast_node_t *current_node = ast;

    while (current_node != NULL) {
        // Check if node is function definition
        if (current_node->type == AST_NODE_FUN_DEFINITION) {
            // Search for the node with the same name/key
            symtable_node_t *defined_node = symtable_stack_search_current(
                symtable_stack->global, SYM_TYPE_FUNC, current_node->fun_definition.name);

            if (defined_node == NULL) {
                // Error code 4, redefinition/overloading of function unsupported
                return false;
            }

            symtable_data_t fun_data;

            // Initialize values of struct symtable_data_t
            fun_data.type = SYM_TYPE_FUNC;
            fun_data.fun.param_count = count_fun_params(current_node->fun_definition.params);
            fun_data.fun.params = current_node->fun_definition.params;
            fun_data.fun.return_type = current_node->fun_definition.return_type;

            // Insert new entry of ID to global symtable
            symtable_insert_item(symtable_stack, SYM_TYPE_FUNC, current_node->fun_definition.name,
                                 fun_data);
        } else {
            // Node is something else, not important in first passage
            current_node = ast->next;
        }
    }

    return true;
}

void second_ast_passage(ast_node_t *ast, symtable_stack_t *symtable_stack) {}

bool semantic_analysis(ast_node_t *ast, symtable_stack_t *symtable_stack) {
    // First passage looks for global IDs and assigns them to global symtable
    if (!first_ast_passage(ast, symtable_stack)) {
        return false;
    }
    second_ast_passage(ast, symtable_stack);
    return true;
}
