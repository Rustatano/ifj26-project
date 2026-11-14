/*
-------------------------------------------------------
Name of project: Translator IFJ26
Authors: xsvoboj01 (Jakub Svoboda)
Creation date: 18.10.2026
File: semantic.c

Description: Implementation of semantic analyzer

-------------------------------------------------------
*/

#include "semantic.h"
#include <stdio.h>

static int semantic_error_code = 0;

void report_semantic_error(semantic_context_t *ctx, int code, const char *msg, int line) {
    (void)ctx; // Unsused context
    if (semantic_error_code == 0) {
        semantic_error_code = code;
        fprintf(stderr, "Semantic analysis Error %d: %s on line %d\n", code, msg, line);
    }
}

// Initialize global symtable
bool semantic_init(semantic_context_t *ctx, symtable_stack_t *symtable_stack) {
    if (symtable_stack == NULL) {
        report_semantic_error(ctx, 99, "Internal error: null symbol table stack", 0);
        return false;
    }

    if (!symtable_init(symtable_stack)) {
        report_semantic_error(ctx, 99, "Internal error: symbol table initialization failed", 0);
        return false;
    }

    if (!symtable_create_avl(symtable_stack)) {
        report_semantic_error(ctx, 99, "Internal error: symbol table AVL creation failed", 0);
        return false;
    }

    return true;
}

// Count function parameters recursively
int count_fun_params(semantic_context_t *ctx, symtable_fun_param_t *param) {
    if (param == NULL) {
        return 0;
    }

    return count_fun_params(ctx, param->next) + 1;
}

// Check if variable is defined
symtable_datatype_t check_expres_var(semantic_context_t *ctx, symtable_stack_t *symtable_stack,
                                     ast_node_t *node) {
    symtable_node_t *var_declaration =
        symtable_search(symtable_stack, SYM_TYPE_VAR, node->expression_var.name);
    if (var_declaration == NULL) {
        report_semantic_error(ctx, 3, "Use of undefined variable", node->source_code_line);
        return SYM_DATATYPE_ERROR;
    }

    return var_declaration->data.var.type;
}

// Check if function is defined and calling is correct
symtable_datatype_t check_expres_fun_call(semantic_context_t *ctx, symtable_stack_t *symtable_stack,
                                          ast_node_t *node) {
    symtable_node_t *fun_definition =
        symtable_search(symtable_stack, SYM_TYPE_FUNC, node->expression_fun_call.fun_name);
    if (fun_definition == NULL) {
        report_semantic_error(ctx, 3, "Use of undefined function", node->source_code_line);
        return SYM_DATATYPE_ERROR;
    }

    symtable_fun_param_t *defined_param = fun_definition->data.fun.params;
    ast_node_t *checked_param = node->expression_fun_call.params; // Linked list

    while (defined_param != NULL) {
        if (checked_param == NULL) {
            report_semantic_error(ctx, 5, "Too few arguments in function call",
                                  node->source_code_line);
            return SYM_DATATYPE_ERROR;
        }
        symtable_datatype_t arg_type = check_expr(ctx, symtable_stack, checked_param);
        if (arg_type == SYM_DATATYPE_ERROR) {
            return SYM_DATATYPE_ERROR;
        }
        if (defined_param->type != arg_type) {
            if (defined_param->type == SYM_DATATYPE_DOUBLE && arg_type == SYM_DATATYPE_INT) {
                if (implicit_conversion_int_to_double(ctx, &checked_param) == SYM_DATATYPE_ERROR) {
                    report_semantic_error(ctx, 6, "Type mismatch in function call argument",
                                          node->source_code_line);
                    return SYM_DATATYPE_ERROR;
                }
            } else {
                report_semantic_error(ctx, 5, "Argument type mismatch in function call",
                                      node->source_code_line);
                return SYM_DATATYPE_ERROR;
            }
        }
        defined_param = defined_param->next;
        checked_param = checked_param->next;
    }

    if (checked_param != NULL) {
        report_semantic_error(ctx, 5, "Too many arguments in function call",
                              node->source_code_line);
        return SYM_DATATYPE_ERROR;
    }

    return fun_definition->data.fun.return_type;
}

// Allocate and initialize AST_NODE_CONVERT
ast_node_t *create_conversion_node(semantic_context_t *ctx, ast_node_t *from) {
    ast_node_t *new_node = (ast_node_t *)malloc(sizeof(ast_node_t));
    if (new_node == NULL) {
        report_semantic_error(ctx, 99, "Internal error: memory allocation failed",
                              from ? from->source_code_line : 0);
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
symtable_datatype_t implicit_conversion_int_to_double(semantic_context_t *ctx,
                                                      ast_node_t **node_ptr) {
    if (node_ptr == NULL || *node_ptr == NULL) {
        return SYM_DATATYPE_ERROR;
    }

    ast_node_t *node = *node_ptr;

    // Convert constant
    if (node->type == AST_NODE_CONST_INT) {
        node->type = AST_NODE_CONST_DOUBLE;
        node->double_value = (double)node->int_value;
        node->result_type = SYM_DATATYPE_DOUBLE;
        return SYM_DATATYPE_DOUBLE;
    }

    // Convert node of result type int
    if (node->result_type == SYM_DATATYPE_INT) {
        ast_node_t *conversion_node = create_conversion_node(ctx, node);
        if (conversion_node == NULL) {
            return SYM_DATATYPE_ERROR;
        }

        // Connect new node
        *node_ptr = conversion_node;
        return SYM_DATATYPE_DOUBLE;
    }

    // Already double
    if (node->result_type == SYM_DATATYPE_DOUBLE) {
        return SYM_DATATYPE_DOUBLE;
    }

    return SYM_DATATYPE_ERROR;
}

// Check if left and right side of operator has the same type
symtable_datatype_t check_expres_operator(semantic_context_t *ctx, symtable_stack_t *symtable_stack,
                                          ast_node_t *node) {
    // Get types of left and right operands
    symtable_datatype_t left = check_expr(ctx, symtable_stack, node->expression_operator.left_side);
    if (left == SYM_DATATYPE_ERROR)
        return SYM_DATATYPE_ERROR;
    symtable_datatype_t right = check_expr(symtable_stack, node->expression_operator.right_side);
    if (right == SYM_DATATYPE_ERROR)
        return SYM_DATATYPE_ERROR;

    // Check operands types correctness
    if (left == SYM_DATATYPE_INT && right == SYM_DATATYPE_DOUBLE) {
        if (implicit_conversion_int_to_double(ctx, &(node->expression_operator.left_side)) ==
            SYM_DATATYPE_ERROR) {
            report_semantic_error(ctx, 6, "Type mismatch in expression", node->source_code_line);
            return SYM_DATATYPE_ERROR;
        }
        return SYM_DATATYPE_DOUBLE;
    } else if (left == SYM_DATATYPE_DOUBLE && right == SYM_DATATYPE_INT) {
        if (implicit_conversion_int_to_double(ctx, &(node->expression_operator.right_side)) ==
            SYM_DATATYPE_ERROR) {
            report_semantic_error(ctx, 6, "Type mismatch in expression", node->source_code_line);
            return SYM_DATATYPE_ERROR;
        }
        return SYM_DATATYPE_DOUBLE;
    } else if (left == right) {
        return left;
    } else {
        report_semantic_error(ctx, 6, "Type mismatch in expression operands",
                              node->source_code_line);
        return SYM_DATATYPE_ERROR;
    }
}

// Determine type of expression recursively
symtable_datatype_t check_expr(semantic_context_t *ctx, symtable_stack_t *symtable_stack,
                               ast_node_t *node) {
    if (node == NULL)
        return SYM_DATATYPE_ERROR;
    switch (node->type) {
    case AST_NODE_EXPRESSION_OPERATOR:
        return node->result_type = check_expres_operator(ctx, symtable_stack, node);

    case AST_NODE_EXPRESSION_FUN_CALL:
        return node->result_type = check_expres_fun_call(ctx, symtable_stack, node);

    case AST_NODE_EXPRESSION_VAR:
        return node->result_type = check_expres_var(ctx, symtable_stack, node);

    case AST_NODE_CONST_INT:
    case AST_NODE_CONST_DOUBLE:
    case AST_NODE_CONST_STRING:
    case AST_NODE_CONST_UNIT:
        return node->result_type;
    default:
        report_semantic_error(ctx, 10, "Invalid expression node type", node->source_code_line);
        return SYM_DATATYPE_ERROR;
    }
}

// Check if variable is not declared and initialize it
symtable_datatype_t check_var_declaration(semantic_context_t *ctx, symtable_stack_t *symtable_stack,
                                          ast_node_t *node) {
    // Search for variable declaration in current scope only because shadowing is allowed
    symtable_node_t *var_declaration =
        symtable_stack_search_current(symtable_stack, SYM_TYPE_VAR, node->var_declaration.name);

    if (var_declaration != NULL) {
        report_semantic_error(ctx, 4, "Redefinition of variable in the same scope",
                              node->source_code_line);
        return SYM_DATATYPE_ERROR;
    }

    // Initialize symtable node data
    symtable_data_t var_data;
    var_data.type = SYM_TYPE_VAR;
    var_data.var.is_mutable = node->var_declaration.is_mutable;
    var_data.var.type = node->var_declaration.var_type;
    var_data.var.unique_id = ctx->unique_var_id;
    ctx->unique_var_id++;

    // Create new entry in symtable for new variable
    if (!symtable_insert_item(symtable_stack, node->var_declaration.name, var_data)) {
        report_semantic_error(ctx, 99,
                              "Internal error: symbol table item (variable) insertion failed",
                              node->source_code_line);
        return SYM_DATATYPE_ERROR;
    }

    // Declaration of variable si invalid, must be initialized
    if (node->var_declaration.init_expression == NULL) {
        report_semantic_error(ctx, 6, "Variable must be initialized at declaration",
                              node->source_code_line);
        return SYM_DATATYPE_ERROR;
    }

    // Get initialization expression type
    symtable_datatype_t init_expr_type =
        check_expr(ctx, symtable_stack, node->var_declaration.init_expression);
    if (init_expr_type == SYM_DATATYPE_ERROR) {
        return SYM_DATATYPE_ERROR;
    }

    // Type of variable is not determined from parser
    if (node->var_declaration.var_type == SYM_DATATYPE_UNKNOWN) {
        return node->var_declaration.var_type = init_expr_type;
    }

    // Type of variable is determined from parser
    if (node->var_declaration.var_type != init_expr_type) {
        if (node->var_declaration.var_type == SYM_DATATYPE_DOUBLE &&
            init_expr_type == SYM_DATATYPE_INT) {
            if (implicit_conversion_int_to_double(ctx, &node->var_declaration.init_expression) ==
                SYM_DATATYPE_ERROR) {
                report_semantic_error(ctx, 6, "Type mismatch in variable initialization",
                                      node->source_code_line);
                return SYM_DATATYPE_ERROR;
            }
            return SYM_DATATYPE_DOUBLE;
        } else {
            report_semantic_error(ctx, 6, "Type mismatch in variable initialization",
                                  node->source_code_line);
            return SYM_DATATYPE_ERROR;
        }
    }

    return init_expr_type;
}

// Check if variable is declared and assign to it
symtable_datatype_t check_value_assignment(semantic_context_t *ctx,
                                           symtable_stack_t *symtable_stack, ast_node_t *node) {
    // Search for variabel to assign to
    symtable_node_t *var_declaration =
        symtable_search(symtable_stack, SYM_TYPE_VAR, node->assign_statement.target_var_name);

    if (var_declaration == NULL) {
        report_semantic_error(ctx, 3, "Assignment to undefined variable", node->source_code_line);
        return SYM_DATATYPE_ERROR;
    }

    if (!var_declaration->data.var.is_mutable) {
        report_semantic_error(ctx, 7, "Assignment to immutable variable", node->source_code_line);
        return SYM_DATATYPE_ERROR;
    }

    symtable_datatype_t expr_type =
        check_expr(ctx, symtable_stack, node->assign_statement.expression);
    if (expr_type == SYM_DATATYPE_ERROR) {
        return SYM_DATATYPE_ERROR;
    }

    if (var_declaration->data.var.type != expr_type) {
        if (var_declaration->data.var.type == SYM_DATATYPE_DOUBLE &&
            expr_type == SYM_DATATYPE_INT) {
            if (implicit_conversion_int_to_double(ctx, &node->assign_statement.expression) ==
                SYM_DATATYPE_ERROR) {
                report_semantic_error(ctx, 6, "Type mismatch in assignment",
                                      node->source_code_line);
                return SYM_DATATYPE_ERROR;
            }
        } else {
            report_semantic_error(ctx, 6, "Type mismatch in assignment", node->source_code_line);
            return SYM_DATATYPE_ERROR;
        }
    }

    return SYM_DATATYPE_UNIT;
}

// Check if statement blocks and condition
symtable_datatype_t check_statement_if(semantic_context_t *ctx, symtable_stack_t *symtable_stack,
                                       ast_node_t *node, symtable_node_t *fun_context) {
    if (check_expr(ctx, symtable_stack, node->if_statement.condition) == SYM_DATATYPE_ERROR) {
        return SYM_DATATYPE_ERROR;
    }
    if (check_block(ctx, symtable_stack, node->if_statement.if_block, fun_context) ==
        SYM_DATATYPE_ERROR) {
        return SYM_DATATYPE_ERROR;
    }
    if (node->if_statement.else_block != NULL) {
        if (check_block(ctx, symtable_stack, node->if_statement.else_block, fun_context) ==
            SYM_DATATYPE_ERROR) {
            return SYM_DATATYPE_ERROR;
        }
    }

    return SYM_DATATYPE_UNIT;
}

// Check while statement blocks and condition
symtable_datatype_t check_statement_while(semantic_context_t *ctx, symtable_stack_t *symtable_stack,
                                          ast_node_t *node, symtable_node_t *fun_context) {
    if (check_expr(ctx, symtable_stack, node->while_statement.condition) == SYM_DATATYPE_ERROR) {
        return SYM_DATATYPE_ERROR;
    }
    if (check_block(ctx, symtable_stack, node->while_statement.body_block, fun_context) ==
        SYM_DATATYPE_ERROR) {
        return SYM_DATATYPE_ERROR;
    }
    return SYM_DATATYPE_UNIT;
}

// Check correctness of return type
symtable_datatype_t check_statement_return(semantic_context_t *ctx,
                                           symtable_stack_t *symtable_stack, ast_node_t *node,
                                           symtable_node_t *fun_context) {
    symtable_datatype_t return_type =
        check_expr(ctx, symtable_stack, node->return_statement.expression);
    if (return_type == SYM_DATATYPE_ERROR) {
        return SYM_DATATYPE_ERROR;
    }

    if (fun_context->data.fun.return_type != return_type) {
        if (fun_context->data.fun.return_type == SYM_DATATYPE_DOUBLE &&
            return_type == SYM_DATATYPE_INT) {
            if (implicit_conversion_int_to_double(ctx, &node->return_statement.expression) ==
                SYM_DATATYPE_ERROR) {
                report_semantic_error(ctx, 6, "Return type mismatch", node->source_code_line);
                return SYM_DATATYPE_ERROR;
            }
        } else {
            report_semantic_error(ctx, 6, "Return type mismatch", node->source_code_line);
            return SYM_DATATYPE_ERROR;
        }
    }

    return return_type;
}

// Check statements in code block sequentially
symtable_datatype_t check_block(semantic_context_t *ctx, symtable_stack_t *symtable_stack,
                                ast_node_t *node, symtable_node_t *fun_context) {
    if (node == NULL)
        return SYM_DATATYPE_UNIT;
    if (!symtable_create_avl(symtable_stack)) {
        report_semantic_error(ctx, 99, "Internal error: symbol table AVL creation failed",
                              node->source_code_line);
        return SYM_DATATYPE_ERROR;
    }

    // Assign first statement of the block
    ast_node_t *current_node = node->block.first_statement;

    while (current_node != NULL) {
        switch (current_node->type) {
        case AST_NODE_VAR_DECLARATION:
            if ((current_node->result_type = check_var_declaration(
                     ctx, symtable_stack, current_node)) == SYM_DATATYPE_ERROR) {
                symtable_pop_avl(symtable_stack);
                return SYM_DATATYPE_ERROR;
            }
            break;
        case AST_NODE_VALUE_ASSIGNMENT:
            if ((current_node->result_type = check_value_assignment(
                     ctx, symtable_stack, current_node)) == SYM_DATATYPE_ERROR) {
                symtable_pop_avl(symtable_stack);
                return SYM_DATATYPE_ERROR;
            }
            break;
        case AST_NODE_STATEMENT_IF:
            if ((current_node->result_type = check_statement_if(
                     ctx, symtable_stack, current_node, fun_context)) == SYM_DATATYPE_ERROR) {
                symtable_pop_avl(symtable_stack);
                return SYM_DATATYPE_ERROR;
            }
            break;
        case AST_NODE_STATEMENT_WHILE:
            if ((current_node->result_type = check_statement_while(
                     ctx, symtable_stack, current_node, fun_context)) == SYM_DATATYPE_ERROR) {
                symtable_pop_avl(symtable_stack);
                return SYM_DATATYPE_ERROR;
            }
            break;
        case AST_NODE_STATEMENT_RETURN:
            if ((current_node->result_type = check_statement_return(
                     ctx, symtable_stack, current_node, fun_context)) == SYM_DATATYPE_ERROR) {
                symtable_pop_avl(symtable_stack);
                return SYM_DATATYPE_ERROR;
            }
            break;
        case AST_NODE_EXPRESSION_FUN_CALL:
            if ((current_node->result_type = check_expres_fun_call(
                     ctx, symtable_stack, current_node)) == SYM_DATATYPE_ERROR) {
                symtable_pop_avl(symtable_stack);
                return SYM_DATATYPE_ERROR;
            }
            break;
        default:
            report_semantic_error(ctx, 10, "Invalid statement type",
                                  current_node->source_code_line);
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

// First passage of AST saves global definitions of functions
bool first_ast_passage(semantic_context_t *ctx, ast_node_t *ast, symtable_stack_t *symtable_stack) {
    if (ast == NULL || symtable_stack == NULL) {
        return false;
    }

    ast_node_t *current_node = ast;
    bool main_found = false;

    while (current_node != NULL) {
        // Check if node is function definition
        if (current_node->type == AST_NODE_FUN_DEFINITION) {
            // Search for the node with the same name/key
            symtable_node_t *defined_node =
                symtable_search(symtable_stack, SYM_TYPE_FUNC, current_node->fun_definition.name);

            if (defined_node != NULL) {
                report_semantic_error(ctx, 4, "Redefinition of function",
                                      current_node->source_code_line);
                return false;
            }

            symtable_data_t fun_data;

            // Initialize values of struct symtable_data_t
            fun_data.type = SYM_TYPE_FUNC;
            fun_data.fun.param_count = count_fun_params(ctx, current_node->fun_definition.params);
            fun_data.fun.params = current_node->fun_definition.params;
            fun_data.fun.return_type = current_node->fun_definition.return_type;

            // Insert new entry of function to global symtable
            if (!symtable_insert_item(symtable_stack, current_node->fun_definition.name,
                                      fun_data)) {
                report_semantic_error(
                    ctx, 99, "Internal error: symbol table item (function) insertion failed",
                    current_node->source_code_line);
                return false;
            }
        } else {
            // Node '@main' annotaion or something else unimportant
            if (current_node->type == AST_NODE_MAIN) {
                if (current_node->type == AST_NODE_MAIN) {
                    if (main_found == true) {
                        report_semantic_error(ctx, 4, "Duplicate main function declaration",
                                              current_node->source_code_line);
                        return false;
                    }
                    main_found = true;
                }
                current_node = current_node->next;
            }
        }

        // Check if main function was declared
        if (main_found == false) {
            report_semantic_error(ctx, 3, "Missing main function", 0);
            return false;
        }

        return true;
    }

    // Second pasasge of AST checks every block and statement
    bool second_ast_passage(semantic_context_t * ctx, ast_node_t * ast,
                            symtable_stack_t * symtable_stack) {
        if (ast == NULL || symtable_stack == NULL) {
            return false;
        }

        ast_node_t *current_node = ast;

        symtable_node_t *fun_context;

        while (current_node != NULL) {
            if (current_node->type == AST_NODE_FUN_DEFINITION) {
                fun_context = symtable_stack_search_current(symtable_stack, SYM_TYPE_FUNC,
                                                            current_node->fun_definition.name);
                if (check_block(ctx, symtable_stack, current_node->fun_definition.body_block,
                                fun_context) == SYM_DATATYPE_ERROR) {
                    return false;
                }
            }

            // New current node
            if (current_node->type == AST_NODE_MAIN) {
                if (current_node->type == AST_NODE_MAIN) {
                    current_node = current_node->block.first_statement;
                } else {
                    current_node = current_node->next;
                }
            }

            return true;
        }

        // Wrapper function to call in main()
        bool semantic_analysis(ast_node_t * ast, symtable_stack_t * symtable_stack) {
            semantic_error_code = 0;
            semantic_context_t ctx;
            ctx.unique_var_id = 0;

            if (ast == NULL || symtable_stack == NULL) {
                report_semantic_error(&ctx, 99, "Internal error: null AST or symbol table stack",
                                      0);
                return false;
            }
            if (!semantic_init(&ctx, symtable_stack)) {
                symtable_stack_delete(symtable_stack);
                return false;
            }
            if (!first_ast_passage(&ctx, ast, symtable_stack)) {
                symtable_stack_delete(symtable_stack);
                return false;
            }
            if (!second_ast_passage(&ctx, ast, symtable_stack)) {
                symtable_stack_delete(symtable_stack);
                return false;
            }

            return true;
        }
