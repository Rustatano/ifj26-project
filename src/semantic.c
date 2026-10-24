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

symtable_datatype_t check_expres_fun_call(symtable_stack_t *symtable_stack, ast_node_t *node) {
    symtable_node_t *fun_definition =
        symtable_search(symtable_stack, SYM_TYPE_FUNC, node->expres_fun_call.fun_name);
    if (fun_definition == NULL) {
        // Semantic error
        return SYM_DATATYPE_ERROR;
    }

    symtable_fun_param_t *defined_param = fun_definition->data.fun.params;
    ast_node_t *checked_param = node->expres_fun_call.params; // Linked list

    while (defined_param != NULL) {
        if (checked_param == NULL) {
            return SYM_DATATYPE_ERROR;
        }
        if (defined_param->type != check_expr(symtable_stack, checked_param)) {
            return SYM_DATATYPE_ERROR;
        }
        defined_param = defined_param->next;
        checked_param = checked_param->next;
    }

    if (checked_param != NULL) {
        return SYM_DATATYPE_ERROR;
    }

    return fun_definition->data.fun.return_type;
}

symtable_datatype_t check_expres_operator_add(symtable_stack_t *symtable_stack, ast_node_t *node) {
    symtable_datatype_t left = check_expr(symtable_stack, node->expres_operator.left_side);
    symtable_datatype_t right = check_expr(symtable_stack, node->expres_operator.right_side);
    if (left != right) {
        // Semantic Error
        return SYM_DATATYPE_ERROR;
    }

    node->result_type = left; // left == right

    return left;
}

symtable_datatype_t check_expres_operator(symtable_stack_t *symtable_stack, ast_node_t *node) {
    switch (node->expres_operator.op) {
    case AST_EXPRES_OPERATOR_ADD:
        return check_expres_operator_add(symtable_stack, node);

    case AST_EXPRES_OPERATOR_SUB:
        break;

    case AST_EXPRES_OPERATOR_MUL:
        break;

    case AST_EXPRES_OPERATOR_DIV:
        break;

    case AST_EXPRES_OPERATOR_EQUAL:
        break;

    case AST_EXPRES_OPERATOR_NOT_EQUAL:
        break;

    case AST_EXPRES_OPERATOR_SMALLER:
        break;

    case AST_EXPRES_OPERATOR_SMALLER_EQUAL:
        break;

    case AST_EXPRES_OPERATOR_BIGGER:
        break;

    case AST_EXPRES_OPERATOR_BIGGER_EQUAL:
        break;

    default:
        break;
    }

    return SYM_DATATYPE_ERROR;
}

symtable_datatype_t check_expr(symtable_stack_t *symtable_stack, ast_node_t *node) {
    switch (node->type) {
    case AST_NODE_EXPRES_OPERATOR:
        return check_expres_operator(symtable_stack, node);
    case AST_NODE_EXPRES_FUN_CALL:
        return check_expres_fun_call(symtable_stack, node);
    case AST_NODE_EXPRES_VAR:
        break;

    case AST_NODE_CONST_INT:
    case AST_NODE_CONST_DOUBLE:
    case AST_NODE_CONST_STRING:
    case AST_NODE_CONST_UNIT:
        return node->result_type;

    default:
        break;
    }

    return SYM_DATATYPE_ERROR;
}
bool check_var_declaration(symtable_stack_t *symtable_stack, ast_node_t *node) {
    // Declaration of variable
    if (node->var_declaration.init_expres == NULL) {
        return false;
    }

    // Declaration and initialization of variable
    if (check_expres(symtable_stack, node->var_declaration.init_expres) == SYM_DATATYPE_ERROR) {
        return false;
    }

    return true;
}

bool check_value_assignment(symtable_stack_t *symtable_stack, ast_node_t *node) { return true; }

bool check_statement_if(symtable_stack_t *symtable_stack, ast_node_t *node) { return true; }

bool check_statement_while(symtable_stack_t *symtable_stack, ast_node_t *node) { return true; }

bool check_statement_return(symtable_stack_t *symtable_stack, ast_node_t *node) { return true; }

bool check_block(symtable_stack_t *symtable_stack, ast_node_t *node) {
    if (!symtable_create_avl(symtable_stack)) {
        return false;
    }

    ast_node_t *current_node = node->block.first_statement;

    while (current_node != NULL) {
        switch (current_node->type) {
        case AST_NODE_VAR_DECLARATION:
            if (!check_var_declaration(symtable_stack, current_node)) {
                return false;
            }
            break;
        case AST_NODE_STATEMENT_IF:
            if (!check_statement_if(symtable_stack, current_node)) {
                return false;
            }
            break;
        case AST_NODE_STATEMENT_WHILE:
            if (!check_statement_while(symtable_stack, current_node)) {
                return false;
            }
            break;
        case AST_NODE_STATEMENT_RETURN:
            if (!check_statement_return(symtable_stack, current_node)) {
                return false;
            }
            break;
        case AST_NODE_EXPRES_FUN_CALL:
            if (!check_expres_fun_call(symtable_stack, current_node)) {
                return false;
            }
            break;
        default:
            break;
        }

        // New node
        current_node = current_node->next;
    }

    return true;
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
            check_block(symtable_stack, current_node->fun_definition.body_block);
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
