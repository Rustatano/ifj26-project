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
#define BUILTIN_FUN_LIST                                                                           \
    {"print", "readLine",  "readInt", "readDouble", "toStr",                                       \
     "toInt", "strLength", "subStr",  "ord",        "chr"}
#define BUILTIN_FUN_COUNT 10

typedef struct {
    FILE *file;
    int label_counter;             // used for while and if
    bool is_in_loop;               // used for handling var declaration vs assignment
    bool is_overflow_check_needed; // whether to generate the code for it
    ast_node_t *last_node;         // used for handling function returns
} codegen_context_t;

// forward declaration so it can be used in other functions
bool handle_node(ast_node_t *node, codegen_context_t *context);

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
    context->is_overflow_check_needed = false;
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

void generate_oveflow_check(codegen_context_t *context) {
    fprintf(context->file, "LABEL $OVERFLOW_CHECK\n");
    // assuming result (int) on top of the stack
    fprintf(context->file, "POPS GF@tmp0\n");
    fprintf(context->file, "PUSHS GF@tmp0\n");
    fprintf(context->file, "PUSHS int@%d\n", INT32_MAX);
    fprintf(context->file, "GTS\n");
    fprintf(context->file, "PUSHS bool@true\n");
    fprintf(context->file, "JUMPIFEQS $OVERFLOW_POSITIVE\n");

    fprintf(context->file, "PUSHS GF@tmp0\n");
    fprintf(context->file, "PUSHS int@%d\n", INT32_MIN);
    fprintf(context->file, "LTS\n");
    fprintf(context->file, "PUSHS bool@true\n");
    fprintf(context->file, "JUMPIFEQS $OVERFLOW_NEGATIVE\n");

    // if no overflow, push back the original value and return
    fprintf(context->file, "PUSHS GF@tmp0\n");
    fprintf(context->file, "RETURN\n");

    // positive overflow
    fprintf(context->file, "LABEL $OVERFLOW_POSITIVE\n");
    fprintf(context->file, "PUSHS int@%d\n", INT32_MIN);
    fprintf(context->file, "PUSHS int@2\n");
    fprintf(context->file, "MULS\n");
    fprintf(context->file, "PUSHS GF@tmp0\n");
    fprintf(context->file, "ADDS\n");
    fprintf(context->file, "RETURN\n");

    // negative overflow
    fprintf(context->file, "LABEL $OVERFLOW_NEGATIVE\n");
    fprintf(context->file, "PUSHS int@%d\n", INT32_MAX);
    fprintf(context->file, "PUSHS int@1\n");
    fprintf(context->file, "ADDS\n");
    fprintf(context->file, "PUSHS int@2\n");
    fprintf(context->file, "MULS\n");
    fprintf(context->file, "PUSHS GF@tmp0\n");
    fprintf(context->file, "ADDS\n");
    fprintf(context->file, "RETURN\n");
}

void check_and_correct_inf_overflow(ast_node_t *node, codegen_context_t *context) {
    if (node->expression_operator.left_side->result_type != SYM_DATATYPE_INT ||
        node->expression_operator.right_side->result_type != SYM_DATATYPE_INT) {
        return;
    }
    // only if both sides are int do the check
    context->is_overflow_check_needed = true;
    fprintf(context->file, "CALL $OVERFLOW_CHECK\n");
}

// this is used to extract variable declarations from block nodes
// so there is no multiple var declarations in case of a loop block
void extract_var_declarations(ast_node_t *node, codegen_context_t *context) {
    if (node == NULL) {
        return;
    }

    if (node->type == AST_NODE_VAR_DECLARATION) {
        fprintf(context->file, "DEFVAR LF@%s$%d\n", node->var_declaration.name,
                node->var_declaration.unique_id);
    }

    extract_var_declarations(node->next, context);
}

// check if a function is builtin and needs custom codegen
bool check_function_is_builtin(char *fun_name) {
    const char *builtin_functions[BUILTIN_FUN_COUNT] = BUILTIN_FUN_LIST;

    for (int i = 0; i < BUILTIN_FUN_COUNT; i++) {
        if (strcmp(fun_name, builtin_functions[i]) == 0) {
            return true;
        }
    }

    return false;
}

// generate code for builtin functions in place
void generate_builtin_function_in_place(char *fun_name, ast_node_t *node,
                                        codegen_context_t *context) {
    if (strcmp(fun_name, "print") == 0) {
        // evaluate the param (only one in this case), result is on top of the stack
        handle_node(node->expression_fun_call.params, context);
        // move the value from stack to var, so it can be used with write
        fprintf(context->file, "POPS GF@tmp0\n");
        if (node->expression_fun_call.params->result_type == SYM_DATATYPE_UNIT) {
            // special case, unit should be printed as "()"
            fprintf(context->file, "MOVE GF@tmp0 string@()\n");
        }
        // write the value
        fprintf(context->file, "WRITE GF@tmp0\n");
        // push unit to stack as return of the print function
        fprintf(context->file, "PUSHS nil@nil\n");

    } else if (strcmp(fun_name, "readLine") == 0) {
        fprintf(context->file, "READ GF@tmp0 string\n");
        // push the value to stack because the rest of codegen expects it
        fprintf(context->file, "PUSHS GF@tmp0\n");

    } else if (strcmp(fun_name, "readInt") == 0) {
        fprintf(context->file, "READ GF@tmp0 int\n");
        // push to stack, same as readLine
        fprintf(context->file, "PUSHS GF@tmp0\n");

    } else if (strcmp(fun_name, "readDouble") == 0) {
        fprintf(context->file, "READ GF@tmp0 float\n");
        // push to stack, same as readLine
        fprintf(context->file, "PUSHS GF@tmp0\n");

    } else if (strcmp(fun_name, "toStr") == 0) {
        handle_node(node->expression_fun_call.params, context);

        if (node->expression_fun_call.params->result_type == SYM_DATATYPE_UNIT) {
            // // special case, unit should be converted to "()"
            fprintf(context->file, "POPS GF@tmp0\n");
            fprintf(context->file, "MOVE GF@tmp0 string@()\n");
            fprintf(context->file, "PUSHS GF@tmp0\n");

        } else if (node->expression_fun_call.params->result_type == SYM_DATATYPE_DOUBLE) {
            fprintf(context->file, "FLOAT2STRS\n");

        } else if (node->expression_fun_call.params->result_type == SYM_DATATYPE_INT) {
            fprintf(context->file, "INT2STRS\n");
        } else {
            // for string, we can leave it be
        }

    } else if (strcmp(fun_name, "toInt") == 0) {
        handle_node(node->expression_fun_call.params, context);
        fprintf(context->file, "FLOAT2INTS\n");

    } else if (strcmp(fun_name, "strLength") == 0) {
        handle_node(node->expression_fun_call.params, context);
        fprintf(context->file, "POPS GF@tmp0\n"); // string
        fprintf(context->file, "STRLEN GF@tmp1 GF@tmp0\n");
        // push result to stack
        fprintf(context->file, "PUSHS GF@tmp1\n");

    } else if (strcmp(fun_name, "subStr") == 0) {
        int label_id = context->label_counter;
        context->label_counter++;

        handle_node(node->expression_fun_call.params, context);
        fprintf(context->file, "POPS GF@tmp2\n"); // end index
        fprintf(context->file, "POPS GF@tmp1\n"); // start index
        fprintf(context->file, "POPS GF@tmp0\n"); // string

        // check if params are valid
        // start < 0
        fprintf(context->file, "PUSHS GF@tmp1\n");
        fprintf(context->file, "PUSHS int@0\n");
        fprintf(context->file, "LTS\n");
        fprintf(context->file, "PUSHS bool@true\n");
        fprintf(context->file, "JUMPIFEQS $EXIT40\n");

        // end < 0
        fprintf(context->file, "PUSHS GF@tmp2\n");
        fprintf(context->file, "PUSHS int@0\n");
        fprintf(context->file, "LTS\n");
        fprintf(context->file, "PUSHS bool@true\n");
        fprintf(context->file, "JUMPIFEQS $EXIT40\n");

        // start > end
        fprintf(context->file, "PUSHS GF@tmp1\n");
        fprintf(context->file, "PUSHS GF@tmp2\n");
        fprintf(context->file, "GTS\n");
        fprintf(context->file, "PUSHS bool@true\n");
        fprintf(context->file, "JUMPIFEQS $EXIT40\n");

        // store strLength to gf@tmp3 for further checks
        fprintf(context->file, "STRLEN GF@tmp3 GF@tmp0\n");

        // start >= strLength
        fprintf(context->file, "PUSHS GF@tmp1\n");
        fprintf(context->file, "PUSHS GF@tmp3\n");
        fprintf(context->file, "GTS\n");
        fprintf(context->file, "PUSHS GF@tmp1\n");
        fprintf(context->file, "PUSHS GF@tmp3\n");
        fprintf(context->file, "EQS\n");
        fprintf(context->file, "ORS\n");
        fprintf(context->file, "PUSHS bool@true\n");
        fprintf(context->file, "JUMPIFEQS $EXIT40\n");

        // end > strLength
        fprintf(context->file, "PUSHS GF@tmp2\n");
        fprintf(context->file, "PUSHS GF@tmp3\n");
        fprintf(context->file, "GTS\n");
        fprintf(context->file, "PUSHS bool@true\n");
        fprintf(context->file, "JUMPIFEQS $EXIT40\n");
        // end of checks

        // actual code of subStr
        // sub result is gonna be in GF@tmp3 (strLength is no longer needed)
        // GF@tmp4 is gonna be used for getchar results
        // first char
        fprintf(context->file, "GETCHAR GF@tmp3 GF@tmp0 GF@tmp1\n");
        fprintf(context->file, "ADD GF@tmp1 GF@tmp1 int@1\n");
        // loop for other chars
        fprintf(context->file, "LABEL $SUBSTR_LOOP%d\n", label_id);
        // check if start index has gone over end index
        fprintf(context->file, "PUSHS GF@tmp1\n");
        fprintf(context->file, "PUSHS GF@tmp2\n");
        fprintf(context->file, "GTS\n");
        fprintf(context->file, "PUSHS bool@true\n");
        fprintf(context->file, "JUMPIFEQS $SUBSTR_LOOP_END%d\n", label_id);
        // get current char and concat it
        fprintf(context->file, "GETCHAR GF@tmp4 GF@tmp0 GF@tmp1\n");
        fprintf(context->file, "CONCAT GF@tmp3 GF@tmp3 GF@tmp4\n");
        // increment and jump
        fprintf(context->file, "ADD GF@tmp1 GF@tmp1 int@1\n");
        fprintf(context->file, "JUMP $SUBSTR_LOOP%d\n", label_id);

        // end of loop
        fprintf(context->file, "LABEL $SUBSTR_LOOP_END%d\n", label_id);
        // push result to stack
        fprintf(context->file, "PUSHS GF@tmp3\n");

    } else if (strcmp(fun_name, "ord") == 0) {
        handle_node(node->expression_fun_call.params, context);
        fprintf(context->file, "STR2INTS\n");

    } else if (strcmp(fun_name, "chr") == 0) {
        handle_node(node->expression_fun_call.params, context);
        fprintf(context->file, "INT2CHARS\n");
    }
}

void load_params_from_stack_reverse(symtable_fun_param_t *params, codegen_context_t *context) {
    // end of the linked list
    if (params == NULL) {
        return;
    }
    // first handle the next param
    load_params_from_stack_reverse(params->next, context);
    // then handle the current one
    fprintf(context->file, "POPS LF@%s$%d\n", params->name, params->unique_id);
}

bool handle_node(ast_node_t *node, codegen_context_t *context) {
    if (node == NULL) {
        return true;
    }
    int label_id = context->label_counter;

    switch (node->type) {
    case AST_NODE_MAIN:
        fprintf(context->file, "LABEL $main\n");
        handle_node(node->block.first_statement, context);
        fprintf(context->file, "EXIT int@0\n");
        break;

    case AST_NODE_BLOCK:
        handle_node(node->block.first_statement, context);
        break;

    case AST_NODE_CONVERT:
        // usef for implicit conversion of int to float in expression
        handle_node(node->block.first_statement, context);
        fprintf(context->file, "INT2FLOATS\n");
        break;

    case AST_NODE_FUN_DEFINITION:
        // create label and frame
        fprintf(context->file, "LABEL %s\n", node->fun_definition.name);
        fprintf(context->file, "CREATEFRAME\n");
        fprintf(context->file, "PUSHFRAME\n");

        // save pointer to the params linked list for value assignment from stack
        symtable_fun_param_t *params_ptr = node->fun_definition.params;
        // define params and load them from stack
        while (params_ptr != NULL) {
            fprintf(context->file, "DEFVAR LF@%s$%d\n", params_ptr->name, params_ptr->unique_id);
            params_ptr = params_ptr->next;
        }
        params_ptr = node->fun_definition.params;
        load_params_from_stack_reverse(params_ptr, context);

        // body of the function
        handle_node(node->fun_definition.body_block, context);

        // done, check if return was used
        if (context->last_node == NULL || context->last_node->type != AST_NODE_STATEMENT_RETURN) {
            // check if the last node was a definition, assignment or while
            // if so push unit to stack as return value
            if (context->last_node != NULL &&
                (context->last_node->type == AST_NODE_VAR_DECLARATION ||
                 context->last_node->type == AST_NODE_VALUE_ASSIGNMENT ||
                 context->last_node->type == AST_NODE_STATEMENT_WHILE)) {
                fprintf(context->file, "PUSHS nil@nil\n");
            }
            fprintf(context->file, "RETURN\n");
            fprintf(context->file, "POPFRAME\n");
        }

        // check if unit is function return type
        // if so, push unit to stack as return value
        if (node->fun_definition.return_type == SYM_DATATYPE_UNIT) {
            fprintf(context->file, "PUSHS nil@nil\n");
        }
        break;

    case AST_NODE_VAR_DECLARATION:
        // define the variable if outside a loop
        if (!context->is_in_loop) {
            fprintf(context->file, "DEFVAR LF@%s$%d\n", node->var_declaration.name,
                    node->var_declaration.unique_id);
        }
        if (node->var_declaration.init_expression != NULL) {
            // handle the expression, result is on top of the stack
            handle_node(node->var_declaration.init_expression, context);
            // save the result from the stack to the variable
            fprintf(context->file, "POPS LF@%s$%d\n", node->var_declaration.name,
                    node->var_declaration.unique_id);
        }
        break;

    case AST_NODE_VALUE_ASSIGNMENT:
        // same as variable declaration but just the assignment part
        handle_node(node->assign_statement.expression, context);
        fprintf(context->file, "POPS LF@%s$%d\n", node->assign_statement.target_var_name,
                node->assign_statement.unique_id);
        break;

    case AST_NODE_STATEMENT_IF:
        context->label_counter++;

        // handle condition
        handle_node(node->if_statement.condition, context);
        // push true to stack to compare with the condition
        fprintf(context->file, "PUSHS bool@true\n");
        fprintf(context->file, "JUMPIFEQS $IF_TRUE$%d\n", label_id);
        fprintf(context->file, "JUMP $IF_FALSE$%d\n", label_id);

        // write if block
        fprintf(context->file, "LABEL $IF_TRUE$%d\n", label_id);
        handle_node(node->if_statement.if_block, context);
        fprintf(context->file, "JUMP $IF_END$%d\n", label_id);

        // write else block
        fprintf(context->file, "LABEL $IF_FALSE$%d\n", label_id);
        handle_node(node->if_statement.else_block, context);
        // dont need to jump to end here because the label is here anyway

        // end label
        fprintf(context->file, "LABEL $IF_END$%d\n", label_id);
        break;

    case AST_NODE_STATEMENT_WHILE:
        // set in loop to alter var declaration behavior
        context->is_in_loop = true;
        // extract variable declarations first in case this block is a loop block
        extract_var_declarations(node->next->block.first_statement, context);

        context->label_counter++;
        fprintf(context->file, "JUMP $WHILE_CONDITION$%d\n", label_id);
        fprintf(context->file, "LABEL $WHILE$%d\n", label_id);

        // while body
        handle_node(node->while_statement.body_block, context);

        // while condition
        fprintf(context->file, "LABEL $WHILE_CONDITION$%d\n", label_id);
        handle_node(node->while_statement.condition, context);
        // result of the condition is on top of the stack
        // push true to the stack and check if the condition is true
        fprintf(context->file, "PUSHS bool@true\n");
        fprintf(context->file, "JUMPIFEQS $WHILE$%d\n", label_id);

        context->is_in_loop = false;
        break;

    case AST_NODE_STATEMENT_RETURN:
        handle_node(node->return_statement.expression, context);
        fprintf(context->file, "POPFRAME\n");
        fprintf(context->file, "RETURN\n");
        break;

    case AST_NODE_STATEMENT_IMPORT:
        // dont need do anything here, header is already in startup code
        break;

    case AST_NODE_EXPRESSION_OPERATOR:
        // first handle left and right side
        handle_node(node->expression_operator.left_side, context);
        handle_node(node->expression_operator.right_side, context);

        // then handle operator on the top of the stack
        switch (node->expression_operator.op) {
        case AST_EXPRESSION_OPERATOR_ADD:
            if (node->expression_operator.left_side->result_type == SYM_DATATYPE_STRING &&
                node->expression_operator.right_side->result_type == SYM_DATATYPE_STRING) {
                // because concat does not have stack version, pop the values
                // concat adn put it back to stack
                fprintf(context->file, "POPS GF@tmp0\n");
                fprintf(context->file, "POPS GF@tmp1\n");
                fprintf(context->file, "CONCAT GF@tmp0 GF@tmp1 GF@tmp0\n");
                fprintf(context->file, "PUSHS GF@tmp0\n");
            } else {
                fprintf(context->file, "ADDS\n");
                check_and_correct_inf_overflow(node, context);
            }
            break;

        case AST_EXPRESSION_OPERATOR_SUB:
            fprintf(context->file, "SUBS\n");
            check_and_correct_inf_overflow(node, context);
            break;

        case AST_EXPRESSION_OPERATOR_MUL:
            fprintf(context->file, "MULS\n");
            check_and_correct_inf_overflow(node, context);
            break;

        case AST_EXPRESSION_OPERATOR_DIV:
            if (node->expression_operator.left_side->result_type == SYM_DATATYPE_INT &&
                node->expression_operator.right_side->result_type == SYM_DATATYPE_INT) {
                fprintf(context->file, "IDIVS\n");
                check_and_correct_inf_overflow(node, context);
            } else {
                fprintf(context->file, "DIVS\n");
            }
            break;

        case AST_EXPRESSION_OPERATOR_EQUAL:
            fprintf(context->file, "EQS\n");
            break;

        case AST_EXPRESSION_OPERATOR_NOT_EQUAL:
            fprintf(context->file, "EQS\n");
            fprintf(context->file, "NOTS\n");
            break;

        case AST_EXPRESSION_OPERATOR_SMALLER:
            fprintf(context->file, "LTS\n");
            break;

        case AST_EXPRESSION_OPERATOR_SMALLER_EQUAL:
            // < or =
            // save vars to temp
            fprintf(context->file, "POPS GF@tmp0\n");
            fprintf(context->file, "POPS GF@tmp1\n");
            // push them back for <
            fprintf(context->file, "PUSHS GF@tmp0\n");
            fprintf(context->file, "PUSHS GF@tmp1\n");
            fprintf(context->file, "LTS\n");
            // push them back again for =
            fprintf(context->file, "PUSHS GF@tmp0\n");
            fprintf(context->file, "PUSHS GF@tmp1\n");
            fprintf(context->file, "EQS\n");
            // check if < or = is true
            fprintf(context->file, "ORS\n");
            break;

        case AST_EXPRESSION_OPERATOR_BIGGER:
            fprintf(context->file, "GTS\n");
            break;

        case AST_EXPRESSION_OPERATOR_BIGGER_EQUAL:
            // > or =
            // save vars to temp
            fprintf(context->file, "POPS GF@tmp0\n");
            fprintf(context->file, "POPS GF@tmp1\n");
            // push them back for >
            fprintf(context->file, "PUSHS GF@tmp0\n");
            fprintf(context->file, "PUSHS GF@tmp1\n");
            fprintf(context->file, "GTS\n");
            // push them back again for =
            fprintf(context->file, "PUSHS GF@tmp0\n");
            fprintf(context->file, "PUSHS GF@tmp1\n");
            fprintf(context->file, "EQS\n");
            // check if > or = is true
            fprintf(context->file, "ORS\n");
            break;

        default:
            fprintf(stderr, "Unknown AST operator\n");
            return false;
        }
        break;

    case AST_NODE_EXPRESSION_FUN_CALL:
        // first check if a function is builtin or user defined
        if (check_function_is_builtin(node->expression_fun_call.fun_name)) {
            generate_builtin_function_in_place(node->expression_fun_call.fun_name, node, context);
        } else {
            // params, results will be on stack (in reverse order)
            handle_node(node->expression_fun_call.params, context);
            // call the function
            fprintf(context->file, "CALL %s\n", node->expression_fun_call.fun_name);
        }
        break;

    case AST_NODE_EXPRESSION_VAR:
        fprintf(context->file, "PUSHS LF@%s$%d\n", node->expression_var.name,
                node->expression_var.unique_id);
        break;

    case AST_NODE_CONST_INT:
        fprintf(context->file, "PUSHS int@%d\n", node->int_value);
        break;

    case AST_NODE_CONST_DOUBLE:
        fprintf(context->file, "PUSHS float@%a\n", node->double_value);
        break;

    case AST_NODE_CONST_STRING:
        fprintf(context->file, "PUSHS string@");
        // whitespace, # and \ needs to be written as escape sequence
        for (char *c = node->str_value; *c != '\0'; c++) {
            if ((0 <= *c && *c <= 32) || *c == 35 || *c == 92) {
                fprintf(context->file, "\\%03d", *c);
            } else {
                fprintf(context->file, "%c", *c);
            }
        }
        fprintf(context->file, "\n");
        break;

    case AST_NODE_CONST_UNIT:
        fprintf(context->file, "PUSHS nil@nil\n");
        break;

    default:
        fprintf(stderr, "Unknown AST node\n");
        return false;
    }

    if (node->type != AST_NODE_BLOCK) {
        // we are not interested in the block as all funcs have a block
        context->last_node = node;
    }

    handle_node(node->next, context);
    return true;
}

// main function for code generation
bool codegen(ast_node_t *ast_root, const char *output_file) {
    // create context including the output file
    codegen_context_t *context = create_codegen_context(output_file);
    if (context == NULL) {
        return false;
    }

    // startup code
    fprintf(context->file, ".IFJcode26\n");
    fprintf(context->file, "DEFVAR GF@tmp0\n");
    fprintf(context->file, "DEFVAR GF@tmp1\n");
    fprintf(context->file, "DEFVAR GF@tmp2\n");
    fprintf(context->file, "DEFVAR GF@tmp3\n");
    fprintf(context->file, "DEFVAR GF@tmp4\n");
    fprintf(context->file, "JUMP $main\n");

    // handle AST nodes
    bool result = handle_node(ast_root, context);

    // end of file code
    if (context->is_overflow_check_needed) {
        generate_oveflow_check(context);
    }
    fprintf(context->file, "LABEL $EXIT40\n");
    fprintf(context->file, "EXIT int@40\n");

    // cleanup
    destroy_codegen_context(context);
    return result;
}
