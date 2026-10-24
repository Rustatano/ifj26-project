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
    symtable_node_t *var_declaration =
        symtable_search(symtable_stack, SYM_TYPE_VAR, node->expressions_var.name);
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
            // Try implicit conversion, else emantic error (argument does not match parameter's
            // type)
            return implicit_conversion_int_to_double(&node->assign_statement.expres);
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

// Allocate and initialize AST_NODE_CONVERT
ast_node_t *create_conversion_node(ast_node_t *from) {
    ast_node_t *new_node = (ast_node_t *)malloc(sizeof(ast_node_t));
    if (new_node == NULL) {
        return NULL;
    }

    new_node->type = AST_NODE_CONVERT;
    new_node->source_code_line = from->source_code_line;
    new_node->result_type = SYM_DATATYPE_DOUBLE;
    new_node->next = NULL;
    new_node->block.first_statement = from;

    return new_node;
}

// Do implicit conversion if allowed
symtable_datatype_t implicit_conversion_int_to_double(ast_node_t **node_ptr) {
    if (node_ptr == NULL || *node_ptr == NULL) {
        return false;
    }

    ast_node_t *node = *node_ptr;

    // Convert constant
    if (node->type == AST_NODE_CONST_INT) {
        node->type = AST_NODE_CONST_DOUBLE;
        node->double_value = (double)node->int_value;
        node->result_type = SYM_DATATYPE_DOUBLE;
        return true;
    }

    // Convert node of result type int
    if (node->result_type == SYM_DATATYPE_INT) {
        ast_node_t *conversion_node = create_conversion_node(node);
        if (conversion_node == NULL) {
            return false;
        }

        // Connect new node
        *node_ptr = conversion_node;
        return true;
    }

    // Already double
    if (node->result_type == SYM_DATATYPE_DOUBLE) {
        return SYM_DATATYPE_DOUBLE;
    }

    return SYM_DATATYPE_ERROR;
}

// Check if left and right side of operator has the same type
symtable_datatype_t check_expres_operator(symtable_stack_t *symtable_stack, ast_node_t *node) {
    // Get types of left and right operands
    symtable_datatype_t left = check_expr(symtable_stack, node->expres_operator.left_side);
    symtable_datatype_t right = check_expr(symtable_stack, node->expres_operator.right_side);

    // Check operands types correctness
    if (left == SYM_DATATYPE_INT && right == SYM_DATATYPE_DOUBLE) {
        implicit_conversion_int_to_double(&(node->expres_operator.left_side));
    } else if (left == SYM_DATATYPE_DOUBLE && right == SYM_DATATYPE_INT) {
        implicit_conversion_int_to_double(&(node->expres_operator.right_side));
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
    default:
        return SYM_DATATYPE_ERROR;
    }

    // Semantic error (invalid expression type)
    return SYM_DATATYPE_ERROR;
}

// Check if variable is not declared and initialize it
symtable_datatype_t check_var_declaration(symtable_stack_t *symtable_stack, ast_node_t *node) {
    // Search for variable declaration in current scope only because shadowing is allowed
    symtable_node_t *var_declaration =
        symtable_stack_search_current(symtable_stack, SYM_TYPE_VAR, node->var_declaration.name);

    if (var_declaration != NULL) {
        // Semantic error (re-declaration of variable in the same scope is not allowed)
        return SYM_DATATYPE_ERROR;
    }

    // Declaration of variable si invalid, must be initialized
    if (node->var_declaration.init_expres == NULL) {
        // Semantic error (variable is not initialized at declaration)
        return SYM_DATATYPE_ERROR;
    }

    // Get initialization expression type
    symtable_datatype_t init_expr_type =
        check_expr(symtable_stack, node->var_declaration.init_expres);

    // Type of variable is not determined from parser
    if (node->var_declaration.var_type == SYM_DATATYPE_UNKNOWN) {
        return node->var_declaration.var_type = init_expr_type;
    }

    // Type of variable is determined from parser
    if (node->var_declaration.var_type != init_expr_type) {
        // Try implicit conversion, else semantic error (declaration of variable with type with
        // invalid type expression)
        return implicit_conversion_int_to_double(&node->var_declaration.init_expres);
    }

    return init_expr_type;
}

symtable_datatype_t check_value_assignment(symtable_stack_t *symtable_stack, ast_node_t *node) {
    // Search for variabel to assign to
    symtable_node_t *var_declaration =
        symtable_search(symtable_stack, SYM_TYPE_VAR, node->assign_statement.target_var_name);

    if (var_declaration == NULL) {
        // Semantic error (assignment to undefined variable)
        return SYM_DATATYPE_ERROR;
    }

    if (!var_declaration->data.var.is_mutable) {
        // Semantic error (assignment to immutable variable)
        return SYM_DATATYPE_ERROR;
    }

    if (var_declaration->data.var.type !=
        check_expr(symtable_stack, node->assign_statement.expres)) {
        // Try implicit conversion, else emantic error (assignment of invalid type)
        return implicit_conversion_int_to_double(&node->assign_statement.expres);
    }

    return SYM_DATATYPE_UNIT;
}

symtable_datatype_t check_statement_if(symtable_stack_t *symtable_stack, ast_node_t *node,
                                       symtable_node_t *fun_context) {
    if (check_expr(symtable_stack, node->if_statement.condition)) {
        // Error occured in condition
        return SYM_DATATYPE_ERROR;
    }
    if (check_block(symtable_stack, node->if_statement.if_block, fun_context) ==
        SYM_DATATYPE_ERROR) {
        // Error occured in if block
        return SYM_DATATYPE_ERROR;
    }
    if (check_block(symtable_stack, node->if_statement.else_block, fun_context) ==
        SYM_DATATYPE_ERROR) {
        // Error occured in else block
        return SYM_DATATYPE_ERROR;
    }

    return SYM_DATATYPE_UNIT;
}

symtable_datatype_t check_statement_while(symtable_stack_t *symtable_stack, ast_node_t *node,
                                          symtable_node_t *fun_context) {
    if (check_expr(symtable_stack, node->while_statement.condition)) {
        // Error occured in condition
        return SYM_DATATYPE_ERROR;
    }
    if (check_block(symtable_stack, node->while_statement.body_block, fun_context) ==
        SYM_DATATYPE_ERROR) {
        // Error occured in body block
        return SYM_DATATYPE_ERROR;
    }
    return SYM_DATATYPE_UNIT;
}

symtable_datatype_t check_statement_return(symtable_stack_t *symtable_stack, ast_node_t *node,
                                           symtable_node_t *fun_context) {
    symtable_datatype_t return_type = check_expr(symtable_stack, node->return_statement.expres);
    if (return_type == SYM_DATATYPE_ERROR) {
        // Semantic error (invalid return value)
        return SYM_DATATYPE_ERROR;
    }

    if (fun_context->data.fun.return_type != return_type) {
        // Semantic error (missmatch of return type)
        return SYM_DATATYPE_ERROR;
    }

    return return_type;
}

// Check statements in code block sequentially
symtable_datatype_t check_block(symtable_stack_t *symtable_stack, ast_node_t *node,
                                symtable_node_t *fun_context) {
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
                symtable_pop_avl(symtable_stack);
                return SYM_DATATYPE_ERROR;
            }
            break;
        case AST_NODE_VALUE_ASSIGNMENT:
            if ((current_node->result_type =
                     check_value_assignment(symtable_stack, current_node)) == SYM_DATATYPE_ERROR) {
                symtable_pop_avl(symtable_stack);
                return SYM_DATATYPE_ERROR;
            }
            break;
        case AST_NODE_STATEMENT_IF:
            if ((current_node->result_type = check_statement_if(
                     symtable_stack, current_node, fun_context)) == SYM_DATATYPE_ERROR) {
                symtable_pop_avl(symtable_stack);
                return SYM_DATATYPE_ERROR;
            }
            break;
        case AST_NODE_STATEMENT_WHILE:
            if ((current_node->result_type = check_statement_while(
                     symtable_stack, current_node, fun_context)) == SYM_DATATYPE_ERROR) {
                symtable_pop_avl(symtable_stack);
                return SYM_DATATYPE_ERROR;
            }
            break;
        case AST_NODE_STATEMENT_RETURN:
            if ((current_node->result_type = check_statement_return(
                     symtable_stack, current_node, fun_context)) == SYM_DATATYPE_ERROR) {
                symtable_pop_avl(symtable_stack);
                return SYM_DATATYPE_ERROR;
            }
            break;
        case AST_NODE_EXPRES_FUN_CALL:
            if ((current_node->result_type = check_expres_fun_call(symtable_stack, current_node)) ==
                SYM_DATATYPE_ERROR) {
                symtable_pop_avl(symtable_stack);
                return SYM_DATATYPE_ERROR;
            }
            break;
        default:
            // Semantic error (invalid statement)
            symtable_pop_avl(symtable_stack);
            return SYM_DATATYPE_ERROR;
        }

        // New node
        current_node = current_node->next;
    }

    // Clear current scope
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

            // Insert new entry of function to global symtable
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

    symtable_node_t *fun_context;

    while (current_node != NULL) {
        if (current_node->type == AST_NODE_FUN_DEFINITION) {
            fun_context = symtable_stack_search_current(symtable_stack, SYM_TYPE_FUNC,
                                                        current_node->fun_definition.name);
            if (check_block(symtable_stack, current_node->fun_definition.body_block, fun_context) ==
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
