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

// Check if variable is defined
symtable_datatype_t check_expres_var(symtable_stack_t *symtable_stack, ast_node_t *node) {
    symtable_node_t *var_declaration = symtable_stack_search_current(
        symtable_stack->top, SYM_TYPE_VAR, node->expressions_var.name);
    if (var_declaration == NULL) {
        // Semantic error (usage of not declared variable)
        return SYM_DATATYPE_ERROR;
    }

    return var_declaration->data.var.type;
}

// Check if function is defined and calling is correct
symtable_datatype_t check_expres_fun_call(symtable_stack_t *symtable_stack, ast_node_t *node) {
    symtable_node_t *fun_definition =
        symtable_search(symtable_stack, SYM_TYPE_FUNC, node->expres_fun_call.fun_name);
    if (fun_definition == NULL) {
        // Semantic error (usage of not defined function)
        return SYM_DATATYPE_ERROR;
    }

    symtable_fun_param_t *defined_param = fun_definition->data.fun.params;
    ast_node_t *checked_param = node->expres_fun_call.params; // Linked list

    while (defined_param != NULL) {
        if (checked_param == NULL) {
            // Semantic error (invalid number of parameters, less than defined)
            return SYM_DATATYPE_ERROR;
        }
        if (defined_param->type != check_expr(symtable_stack, checked_param)) {
            // Semantic error (argument does not match parameter's type)
            return SYM_DATATYPE_ERROR;
        }
        defined_param = defined_param->next;
        checked_param = checked_param->next;
    }

    if (checked_param != NULL) {
        // Semantic error (invalid number of parameters, more than defined)
        return SYM_DATATYPE_ERROR;
    }

    return fun_definition->data.fun.return_type;
}

// Check if left and right side of operator has the same type
symtable_datatype_t check_expres_operator(symtable_stack_t *symtable_stack, ast_node_t *node) {
    symtable_datatype_t left = check_expr(symtable_stack, node->expres_operator.left_side);
    symtable_datatype_t right = check_expr(symtable_stack, node->expres_operator.right_side);
    if (left != right) {
        // Semantic Error
        return SYM_DATATYPE_ERROR;
    }

    return left; // left == right
}

// Determine type of expression recursively
symtable_datatype_t check_expr(symtable_stack_t *symtable_stack, ast_node_t *node) {
    switch (node->type) {
    case AST_NODE_EXPRES_OPERATOR:
        return node->result_type = check_expres_operator(symtable_stack, node);

    case AST_NODE_EXPRES_FUN_CALL:
        return node->result_type = check_expres_fun_call(symtable_stack, node);

    case AST_NODE_EXPRES_VAR:
        return node->result_type = check_expres_var(symtable_stack, node);

    case AST_NODE_CONST_INT:
    case AST_NODE_CONST_DOUBLE:
    case AST_NODE_CONST_STRING:
    case AST_NODE_CONST_UNIT:
        return node->result_type;
    }

    // Semantic error (invalid expression type)
    return SYM_DATATYPE_ERROR;
}

// Check
symtable_datatype_t check_var_declaration(symtable_stack_t *symtable_stack, ast_node_t *node) {
    // Declaration of variable si invalid, must be initialized
    if (node->var_declaration.init_expres == NULL) {
        // Semantic error (variable is not initialized at declaration)
        return SYM_DATATYPE_ERROR;
    }

    // Declaration and initialization of variable
    return check_expr(symtable_stack, node->var_declaration.init_expres);
}

bool check_value_assignment(symtable_stack_t *symtable_stack, ast_node_t *node) { return true; }

bool check_statement_if(symtable_stack_t *symtable_stack, ast_node_t *node) { return true; }

bool check_statement_while(symtable_stack_t *symtable_stack, ast_node_t *node) { return true; }

bool check_statement_return(symtable_stack_t *symtable_stack, ast_node_t *node) { return true; }

symtable_datatype_t check_block(symtable_stack_t *symtable_stack, ast_node_t *node) {
    if (!symtable_create_avl(symtable_stack)) {
        // Could not create new symtable
        return SYM_DATATYPE_ERROR;
    }

    // Assign first statement of the block
    ast_node_t *current_node = node->block.first_statement;

    while (current_node != NULL) {
        switch (current_node->type) {
        case AST_NODE_VAR_DECLARATION:
            if ((current_node->result_type = check_var_declaration(symtable_stack, current_node)) ==
                SYM_DATATYPE_ERROR) {
                return SYM_DATATYPE_ERROR;
            }
        case AST_NODE_VALUE_ASSIGNMENT:
            if ((current_node->result_type =
                     check_value_assignment(symtable_stack, current_node)) == SYM_DATATYPE_ERROR) {
                return SYM_DATATYPE_ERROR;
            }
        case AST_NODE_STATEMENT_IF:
            if ((current_node->result_type = check_statement_if(symtable_stack, current_node)) ==
                SYM_DATATYPE_ERROR) {
                return SYM_DATATYPE_ERROR;
            }
        case AST_NODE_STATEMENT_WHILE:
            if ((current_node->result_type = check_statement_while(symtable_stack, current_node)) ==
                SYM_DATATYPE_ERROR) {
                return SYM_DATATYPE_ERROR;
            }
        case AST_NODE_STATEMENT_RETURN:
            if ((current_node->result_type =
                     check_statement_return(symtable_stack, current_node)) == SYM_DATATYPE_ERROR) {
                return SYM_DATATYPE_ERROR;
            }
        case AST_NODE_EXPRES_FUN_CALL:
            if ((current_node->result_type = check_expres_fun_call(symtable_stack, current_node)) ==
                SYM_DATATYPE_ERROR) {
                return SYM_DATATYPE_ERROR;
            }
        default:
            // Semantic error (invalid statement)
            return SYM_DATATYPE_ERROR;
        }

        // New node
        current_node = current_node->next;
    }

    symtable_pop_avl(symtable_stack);

    return SYM_DATATYPE_UNIT;
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
            symtable_node_t *defined_node =
                symtable_search(symtable_stack, SYM_TYPE_FUNC, current_node->fun_definition.name);

            if (defined_node != NULL) {
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
            if (current_node->type == AST_NODE_PROGRAM) {
                current_node = current_node->block.first_statement;
            } else {
                current_node = current_node->next;
            }
        }
    }

    return true;
}

bool second_ast_passage(ast_node_t *ast, symtable_stack_t *symtable_stack) {
    if (ast == NULL || symtable_stack == NULL) {
        return false;
    }

    ast_node_t *current_node = ast;

    symtable_node_t *current_function;

    while (current_node != NULL) {
        if (current_node->type == AST_NODE_FUN_DEFINITION) {
            current_function = symtable_stack_search_current(symtable_stack, SYM_TYPE_FUNC,
                                                             current_node->fun_definition.name);
            if (check_block(symtable_stack, current_node->fun_definition.body_block) ==
                SYM_DATATYPE_ERROR) {
                // Semantic error
                return false;
            }
        }

        // New current node
        if (current_node->type == AST_NODE_PROGRAM) {
            current_node = current_node->block.first_statement;
        } else {
            current_node = current_node->next;
        }
    }

    return true;
}

bool semantic_analysis(ast_node_t *ast, symtable_stack_t *symtable_stack) {
    if (ast == NULL || symtable_stack == NULL) {
        return false;
    }
    // First passage looks for global IDs and assigns them to global symtable
    if (!first_ast_passage(ast, symtable_stack)) {
        return false;
    }
    if (!second_ast_passage(ast, symtable_stack)) {
        return false;
    }

    return true;
}
