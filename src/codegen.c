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
#include <string.h>

// Protocol of the generated code:
//  - every expression leaves exactly one value on the data stack,
//  - every function leaves exactly one return value on the data stack (Unit is nil@nil),
//  - the value of an expression statement is dropped (POPS) unless it is the return value of
//    a function (last statement of its body),
//  - local variables are named LF@name$unique_id, all of them are defined at the beginning
//    of the function (DEFVAR must not be executed twice, e.g. in a loop body).

#define BUILTIN_FUN_LIST                                                                           \
    {"print", "readLine",  "readInt", "readDouble", "toStr",                                       \
     "toInt", "strLength", "subStr",  "ord",        "chr"}
#define BUILTIN_FUN_COUNT 10

typedef struct {
    FILE *file;
    int label_counter;             // used for unique labels (if, while, builtins)
    bool is_overflow_check_needed; // whether to generate the code for it
    bool is_in_main;               // return in the main function ends the program
    bool is_unit_function;         // function returns Unit, so returned values are dropped
    bool failed;                   // error during generation
} codegen_context_t;

// forward declarations
static void generate_expression(ast_node_t *node, codegen_context_t *context);
static void generate_statement_list(ast_node_t *first, ast_node_t *last_value_statement,
                                    codegen_context_t *context);

static codegen_context_t *create_codegen_context(const char *output_file) {
    FILE *file = stdout;
    if (output_file != NULL) {
        file = fopen(output_file, "w");
        if (file == NULL) {
            fprintf(stderr, "Failed to create file\n");
            return NULL;
        }
    }

    codegen_context_t *context = malloc(sizeof(codegen_context_t));
    if (context == NULL) {
        fprintf(stderr, "Failed malloc\n");
        if (file != stdout) {
            fclose(file);
        }
        return NULL;
    }
    context->file = file;
    context->label_counter = 0;
    context->is_overflow_check_needed = false;
    context->is_in_main = false;
    context->is_unit_function = false;
    context->failed = false;
    return context;
}

static void destroy_codegen_context(codegen_context_t *context) {
    if (context != NULL) {
        if (context->file != NULL && context->file != stdout) {
            fclose(context->file);
        } else if (context->file != NULL) {
            fflush(context->file);
        }
        free(context);
    }
}

// Integer operations are done on 64 bit integers, the result is wrapped to 32 bit Int
// (like in Scala). Value is taken from the top of the stack and the wrapped one is pushed back.
static void generate_overflow_check(codegen_context_t *context) {
    FILE *f = context->file;
    fprintf(f, "LABEL $OVERFLOW_CHECK\n");
    fprintf(f, "POPS GF@tmp0\n");
    // tmp0 = tmp0 - (tmp0 / 2^32) * 2^32  -> |tmp0| < 2^32
    fprintf(f, "IDIV GF@tmp1 GF@tmp0 int@4294967296\n");
    fprintf(f, "MUL GF@tmp1 GF@tmp1 int@4294967296\n");
    fprintf(f, "SUB GF@tmp0 GF@tmp0 GF@tmp1\n");
    fprintf(f, "GT GF@tmp1 GF@tmp0 int@%d\n", INT32_MAX);
    fprintf(f, "JUMPIFEQ $OVERFLOW_POSITIVE GF@tmp1 bool@true\n");
    fprintf(f, "LT GF@tmp1 GF@tmp0 int@%d\n", INT32_MIN);
    fprintf(f, "JUMPIFEQ $OVERFLOW_NEGATIVE GF@tmp1 bool@true\n");
    fprintf(f, "PUSHS GF@tmp0\n");
    fprintf(f, "RETURN\n");
    fprintf(f, "LABEL $OVERFLOW_POSITIVE\n");
    fprintf(f, "SUB GF@tmp0 GF@tmp0 int@4294967296\n");
    fprintf(f, "PUSHS GF@tmp0\n");
    fprintf(f, "RETURN\n");
    fprintf(f, "LABEL $OVERFLOW_NEGATIVE\n");
    fprintf(f, "ADD GF@tmp0 GF@tmp0 int@4294967296\n");
    fprintf(f, "PUSHS GF@tmp0\n");
    fprintf(f, "RETURN\n");
}

// Wraps the Int result on the top of the stack, only if both operands were Int
static void generate_int_overflow_check(codegen_context_t *context) {
    context->is_overflow_check_needed = true;
    fprintf(context->file, "CALL $OVERFLOW_CHECK\n");
}

static bool is_int_operation(ast_node_t *node) {
    return node->expression_operator.left_side->result_type == SYM_DATATYPE_INT &&
           node->expression_operator.right_side->result_type == SYM_DATATYPE_INT;
}

// Writes string constant in the format of IFJcode26 (escape sequence \xyz)
static void write_string_constant(const char *str, codegen_context_t *context) {
    fprintf(context->file, "string@");
    for (const unsigned char *p = (const unsigned char *)str; *p != '\0'; p++) {
        if (*p <= 32 || *p == 35 || *p == 92 || *p > 126) {
            fprintf(context->file, "\\%03d", *p);
        } else {
            fputc(*p, context->file);
        }
    }
}

// check if a function is builtin and needs custom codegen
static bool check_function_is_builtin(const char *fun_name) {
    const char *builtin_functions[BUILTIN_FUN_COUNT] = BUILTIN_FUN_LIST;

    for (int i = 0; i < BUILTIN_FUN_COUNT; i++) {
        if (strcmp(fun_name, builtin_functions[i]) == 0) {
            return true;
        }
    }

    return false;
}

// Generates arguments of a call left to right (the last one is on the top of the stack)
static void generate_arguments(ast_node_t *first_argument, codegen_context_t *context) {
    for (ast_node_t *arg = first_argument; arg != NULL; arg = arg->next) {
        generate_expression(arg, context);
    }
}

// generate code for builtin functions in place
static void generate_builtin_function_in_place(const char *fun_name, ast_node_t *node,
                                               codegen_context_t *context) {
    FILE *f = context->file;
    ast_node_t *args = node->expression_fun_call.params;
    int id = context->label_counter++;

    generate_arguments(args, context);

    if (strcmp(fun_name, "print") == 0) {
        // value of the argument is on top of the stack, WRITE needs a variable
        fprintf(f, "POPS GF@tmp0\n");
        if (args->result_type == SYM_DATATYPE_UNIT) {
            // special case, unit should be printed as "()"
            fprintf(f, "MOVE GF@tmp0 string@()\n");
        }
        fprintf(f, "WRITE GF@tmp0\n");
        // result of print is Unit
        fprintf(f, "PUSHS nil@nil\n");

    } else if (strcmp(fun_name, "readLine") == 0) {
        // READ gives nil on EOF, the function returns an empty string then
        fprintf(f, "READ GF@tmp0 string\n");
        fprintf(f, "JUMPIFNEQ $READ_OK$%d GF@tmp0 nil@nil\n", id);
        fprintf(f, "MOVE GF@tmp0 string@\n");
        fprintf(f, "LABEL $READ_OK$%d\n", id);
        fprintf(f, "PUSHS GF@tmp0\n");

    } else if (strcmp(fun_name, "readInt") == 0) {
        // nil (EOF or bad format) -> 0
        fprintf(f, "READ GF@tmp0 int\n");
        fprintf(f, "JUMPIFNEQ $READ_OK$%d GF@tmp0 nil@nil\n", id);
        fprintf(f, "MOVE GF@tmp0 int@0\n");
        fprintf(f, "LABEL $READ_OK$%d\n", id);
        fprintf(f, "PUSHS GF@tmp0\n");

    } else if (strcmp(fun_name, "readDouble") == 0) {
        fprintf(f, "READ GF@tmp0 float\n");
        fprintf(f, "JUMPIFNEQ $READ_OK$%d GF@tmp0 nil@nil\n", id);
        fprintf(f, "MOVE GF@tmp0 float@%a\n", 0.0);
        fprintf(f, "LABEL $READ_OK$%d\n", id);
        fprintf(f, "PUSHS GF@tmp0\n");

    } else if (strcmp(fun_name, "toStr") == 0) {
        if (args->result_type == SYM_DATATYPE_UNIT) {
            // special case, unit should be converted to "()"
            fprintf(f, "POPS GF@tmp0\n");
            fprintf(f, "PUSHS string@()\n");
        } else if (args->result_type == SYM_DATATYPE_DOUBLE) {
            fprintf(f, "FLOAT2STRS\n");
        } else if (args->result_type == SYM_DATATYPE_INT) {
            fprintf(f, "INT2STRS\n");
        }
        // string stays as it is

    } else if (strcmp(fun_name, "toInt") == 0) {
        fprintf(f, "FLOAT2INTS\n");

    } else if (strcmp(fun_name, "strLength") == 0) {
        fprintf(f, "POPS GF@tmp0\n");
        fprintf(f, "STRLEN GF@tmp1 GF@tmp0\n");
        fprintf(f, "PUSHS GF@tmp1\n");

    } else if (strcmp(fun_name, "subStr") == 0) {
        // tmp0 = string, tmp1 = start (also the current index), tmp2 = end,
        // tmp3 = bool / current char, tmp4 = length of the string, tmp5 = result
        fprintf(f, "POPS GF@tmp2\n");
        fprintf(f, "POPS GF@tmp1\n");
        fprintf(f, "POPS GF@tmp0\n");

        // start < 0
        fprintf(f, "LT GF@tmp3 GF@tmp1 int@0\n");
        fprintf(f, "JUMPIFEQ $EXIT40 GF@tmp3 bool@true\n");
        // end < 0
        fprintf(f, "LT GF@tmp3 GF@tmp2 int@0\n");
        fprintf(f, "JUMPIFEQ $EXIT40 GF@tmp3 bool@true\n");
        // start > end
        fprintf(f, "GT GF@tmp3 GF@tmp1 GF@tmp2\n");
        fprintf(f, "JUMPIFEQ $EXIT40 GF@tmp3 bool@true\n");
        // start >= length
        fprintf(f, "STRLEN GF@tmp4 GF@tmp0\n");
        fprintf(f, "LT GF@tmp3 GF@tmp1 GF@tmp4\n");
        fprintf(f, "JUMPIFNEQ $EXIT40 GF@tmp3 bool@true\n");
        // end > length
        fprintf(f, "GT GF@tmp3 GF@tmp2 GF@tmp4\n");
        fprintf(f, "JUMPIFEQ $EXIT40 GF@tmp3 bool@true\n");

        // copy characters start .. end - 1
        fprintf(f, "MOVE GF@tmp5 string@\n");
        fprintf(f, "LABEL $SUBSTR_LOOP$%d\n", id);
        fprintf(f, "LT GF@tmp3 GF@tmp1 GF@tmp2\n");
        fprintf(f, "JUMPIFNEQ $SUBSTR_END$%d GF@tmp3 bool@true\n", id);
        fprintf(f, "GETCHAR GF@tmp3 GF@tmp0 GF@tmp1\n");
        fprintf(f, "CONCAT GF@tmp5 GF@tmp5 GF@tmp3\n");
        fprintf(f, "ADD GF@tmp1 GF@tmp1 int@1\n");
        fprintf(f, "JUMP $SUBSTR_LOOP$%d\n", id);
        fprintf(f, "LABEL $SUBSTR_END$%d\n", id);
        fprintf(f, "PUSHS GF@tmp5\n");

    } else if (strcmp(fun_name, "ord") == 0) {
        // -1 for an empty string or an index out of range
        fprintf(f, "POPS GF@tmp1\n");
        fprintf(f, "POPS GF@tmp0\n");
        fprintf(f, "LT GF@tmp3 GF@tmp1 int@0\n");
        fprintf(f, "JUMPIFEQ $ORD_BAD$%d GF@tmp3 bool@true\n", id);
        fprintf(f, "STRLEN GF@tmp2 GF@tmp0\n");
        fprintf(f, "LT GF@tmp3 GF@tmp1 GF@tmp2\n");
        fprintf(f, "JUMPIFNEQ $ORD_BAD$%d GF@tmp3 bool@true\n", id);
        fprintf(f, "STRI2INT GF@tmp4 GF@tmp0 GF@tmp1\n");
        fprintf(f, "PUSHS GF@tmp4\n");
        fprintf(f, "JUMP $ORD_END$%d\n", id);
        fprintf(f, "LABEL $ORD_BAD$%d\n", id);
        fprintf(f, "PUSHS int@-1\n");
        fprintf(f, "LABEL $ORD_END$%d\n", id);

    } else if (strcmp(fun_name, "chr") == 0) {
        fprintf(f, "INT2CHARS\n");
    }
}

// Generates relational operator, operands are on the stack
static void generate_comparison(ast_expression_operator_t op, codegen_context_t *context) {
    FILE *f = context->file;
    switch (op) {
    case AST_EXPRESSION_OPERATOR_EQUAL:
        fprintf(f, "EQS\n");
        break;
    case AST_EXPRESSION_OPERATOR_NOT_EQUAL:
        fprintf(f, "EQS\n");
        fprintf(f, "NOTS\n");
        break;
    case AST_EXPRESSION_OPERATOR_SMALLER:
        fprintf(f, "LTS\n");
        break;
    case AST_EXPRESSION_OPERATOR_BIGGER:
        fprintf(f, "GTS\n");
        break;
    case AST_EXPRESSION_OPERATOR_SMALLER_EQUAL:
    case AST_EXPRESSION_OPERATOR_BIGGER_EQUAL:
        // a <= b  ==  (a < b) or (a == b),  a >= b  ==  (a > b) or (a == b)
        // stack: [a, b] -> tmp0 = b, tmp1 = a, operands are pushed back in the original order
        fprintf(f, "POPS GF@tmp0\n");
        fprintf(f, "POPS GF@tmp1\n");
        fprintf(f, "PUSHS GF@tmp1\n");
        fprintf(f, "PUSHS GF@tmp0\n");
        fprintf(f, op == AST_EXPRESSION_OPERATOR_SMALLER_EQUAL ? "LTS\n" : "GTS\n");
        fprintf(f, "PUSHS GF@tmp1\n");
        fprintf(f, "PUSHS GF@tmp0\n");
        fprintf(f, "EQS\n");
        fprintf(f, "ORS\n");
        break;
    default:
        break;
    }
}

static void generate_operator(ast_node_t *node, codegen_context_t *context) {
    FILE *f = context->file;
    ast_expression_operator_t op = node->expression_operator.op;
    ast_node_t *left = node->expression_operator.left_side;
    ast_node_t *right = node->expression_operator.right_side;

    // Unary operators
    if (op == AST_EXPRESSION_OPERATOR_UNARY_PLUS) {
        generate_expression(right, context);
        return;
    }
    if (op == AST_EXPRESSION_OPERATOR_UNARY_MINUS) {
        if (right->result_type == SYM_DATATYPE_DOUBLE) {
            generate_expression(right, context);
            fprintf(f, "PUSHS float@%a\n", -1.0);
            fprintf(f, "MULS\n");
        } else {
            fprintf(f, "PUSHS int@0\n");
            generate_expression(right, context);
            fprintf(f, "SUBS\n");
            generate_int_overflow_check(context);
        }
        return;
    }

    // Binary operators: operands first, then the operator on top of the stack
    generate_expression(left, context);
    generate_expression(right, context);

    switch (op) {
    case AST_EXPRESSION_OPERATOR_ADD:
        if (left->result_type == SYM_DATATYPE_STRING) {
            // concat does not have a stack version
            fprintf(f, "POPS GF@tmp0\n");
            fprintf(f, "POPS GF@tmp1\n");
            fprintf(f, "CONCAT GF@tmp0 GF@tmp1 GF@tmp0\n");
            fprintf(f, "PUSHS GF@tmp0\n");
        } else {
            fprintf(f, "ADDS\n");
            if (is_int_operation(node)) {
                generate_int_overflow_check(context);
            }
        }
        break;
    case AST_EXPRESSION_OPERATOR_SUB:
        fprintf(f, "SUBS\n");
        if (is_int_operation(node)) {
            generate_int_overflow_check(context);
        }
        break;
    case AST_EXPRESSION_OPERATOR_MUL:
        fprintf(f, "MULS\n");
        if (is_int_operation(node)) {
            generate_int_overflow_check(context);
        }
        break;
    case AST_EXPRESSION_OPERATOR_DIV:
        if (is_int_operation(node)) {
            fprintf(f, "IDIVS\n");
            generate_int_overflow_check(context); // INT_MIN / -1
        } else {
            fprintf(f, "DIVS\n");
        }
        break;
    default:
        generate_comparison(op, context);
        break;
    }
}

// Generates code of an expression, its value is left on the top of the stack
static void generate_expression(ast_node_t *node, codegen_context_t *context) {
    FILE *f = context->file;
    if (node == NULL) {
        return;
    }

    switch (node->type) {
    case AST_NODE_EXPRESSION_OPERATOR:
        generate_operator(node, context);
        break;

    case AST_NODE_EXPRESSION_FUN_CALL:
        if (check_function_is_builtin(node->expression_fun_call.fun_name)) {
            generate_builtin_function_in_place(node->expression_fun_call.fun_name, node, context);
        } else {
            // arguments are pushed left to right, the callee pops them in reverse order
            generate_arguments(node->expression_fun_call.params, context);
            fprintf(f, "CALL %s\n", node->expression_fun_call.fun_name);
        }
        break;

    case AST_NODE_CONVERT:
        // implicit conversion of Int to Double
        generate_expression(node->block.first_statement, context);
        fprintf(f, "INT2FLOATS\n");
        break;

    case AST_NODE_EXPRESSION_VAR:
        fprintf(f, "PUSHS LF@%s$%d\n", node->expression_var.name, node->expression_var.unique_id);
        break;

    case AST_NODE_CONST_INT:
        fprintf(f, "PUSHS int@%d\n", node->int_value);
        break;

    case AST_NODE_CONST_DOUBLE:
        fprintf(f, "PUSHS float@%a\n", node->double_value);
        break;

    case AST_NODE_CONST_STRING:
        fprintf(f, "PUSHS ");
        write_string_constant(node->str_value, context);
        fprintf(f, "\n");
        break;

    case AST_NODE_CONST_UNIT:
        fprintf(f, "PUSHS nil@nil\n");
        break;

    default:
        fprintf(stderr, "Unknown AST expression node\n");
        context->failed = true;
        break;
    }
}

static bool is_expression_node(ast_node_t *node) {
    switch (node->type) {
    case AST_NODE_EXPRESSION_OPERATOR:
    case AST_NODE_EXPRESSION_FUN_CALL:
    case AST_NODE_EXPRESSION_VAR:
    case AST_NODE_CONST_INT:
    case AST_NODE_CONST_DOUBLE:
    case AST_NODE_CONST_STRING:
    case AST_NODE_CONST_UNIT:
    case AST_NODE_CONVERT:
        return true;
    default:
        return false;
    }
}

// Defines all variables declared in the list of statements (including nested blocks) - each
// variable has a unique id, so one DEFVAR per variable is enough even for loops
static void define_local_variables(ast_node_t *first, codegen_context_t *context) {
    for (ast_node_t *node = first; node != NULL; node = node->next) {
        switch (node->type) {
        case AST_NODE_VAR_DECLARATION:
            fprintf(context->file, "DEFVAR LF@%s$%d\n", node->var_declaration.name,
                    node->var_declaration.unique_id);
            break;
        case AST_NODE_STATEMENT_IF:
            if (node->if_statement.if_block != NULL) {
                define_local_variables(node->if_statement.if_block->block.first_statement, context);
            }
            if (node->if_statement.else_block != NULL) {
                define_local_variables(node->if_statement.else_block->block.first_statement,
                                       context);
            }
            break;
        case AST_NODE_STATEMENT_WHILE:
            if (node->while_statement.body_block != NULL) {
                define_local_variables(node->while_statement.body_block->block.first_statement,
                                       context);
            }
            break;
        default:
            break;
        }
    }
}

// Generates one statement. The value of an expression statement is dropped unless it is
// `keep_value` (the value of the function body).
static void generate_statement(ast_node_t *node, bool keep_value, codegen_context_t *context) {
    FILE *f = context->file;

    if (is_expression_node(node)) {
        generate_expression(node, context);
        if (!keep_value) {
            fprintf(f, "POPS GF@tmp0\n");
        }
        return;
    }

    switch (node->type) {
    case AST_NODE_STATEMENT_IMPORT:
        // header is already in the startup code
        break;

    case AST_NODE_VAR_DECLARATION:
        // variable is already defined at the beginning of the function
        if (node->var_declaration.init_expression != NULL) {
            generate_expression(node->var_declaration.init_expression, context);
            fprintf(f, "POPS LF@%s$%d\n", node->var_declaration.name,
                    node->var_declaration.unique_id);
        }
        break;

    case AST_NODE_VALUE_ASSIGNMENT:
        generate_expression(node->assign_statement.expression, context);
        fprintf(f, "POPS LF@%s$%d\n", node->assign_statement.target_var_name,
                node->assign_statement.unique_id);
        break;

    case AST_NODE_STATEMENT_IF: {
        int id = context->label_counter++;
        generate_expression(node->if_statement.condition, context);
        fprintf(f, "PUSHS bool@true\n");
        fprintf(f, "JUMPIFEQS $IF_TRUE$%d\n", id);
        fprintf(f, "JUMP $IF_FALSE$%d\n", id);

        fprintf(f, "LABEL $IF_TRUE$%d\n", id);
        if (node->if_statement.if_block != NULL) {
            generate_statement_list(node->if_statement.if_block->block.first_statement, NULL,
                                    context);
        }
        fprintf(f, "JUMP $IF_END$%d\n", id);

        fprintf(f, "LABEL $IF_FALSE$%d\n", id);
        if (node->if_statement.else_block != NULL) {
            generate_statement_list(node->if_statement.else_block->block.first_statement, NULL,
                                    context);
        }
        fprintf(f, "LABEL $IF_END$%d\n", id);
        break;
    }

    case AST_NODE_STATEMENT_WHILE: {
        int id = context->label_counter++;
        // the condition is tested before every iteration, including the first one
        fprintf(f, "LABEL $WHILE$%d\n", id);
        generate_expression(node->while_statement.condition, context);
        fprintf(f, "PUSHS bool@true\n");
        fprintf(f, "JUMPIFNEQS $WHILE_END$%d\n", id);
        if (node->while_statement.body_block != NULL) {
            generate_statement_list(node->while_statement.body_block->block.first_statement, NULL,
                                    context);
        }
        fprintf(f, "JUMP $WHILE$%d\n", id);
        fprintf(f, "LABEL $WHILE_END$%d\n", id);
        break;
    }

    case AST_NODE_STATEMENT_RETURN:
        if (context->is_in_main) {
            // return from the main function ends the program
            fprintf(f, "EXIT int@0\n");
            break;
        }
        if (node->return_statement.expression != NULL) {
            generate_expression(node->return_statement.expression, context);
            if (context->is_unit_function) {
                // value is dropped in functions returning Unit
                fprintf(f, "POPS GF@tmp0\n");
                fprintf(f, "PUSHS nil@nil\n");
            }
        } else {
            fprintf(f, "PUSHS nil@nil\n");
        }
        fprintf(f, "POPFRAME\n");
        fprintf(f, "RETURN\n");
        break;

    default:
        fprintf(stderr, "Unknown AST node\n");
        context->failed = true;
        break;
    }
}

// Generates a list of statements; last_value_statement is the statement whose value is kept
static void generate_statement_list(ast_node_t *first, ast_node_t *last_value_statement,
                                    codegen_context_t *context) {
    for (ast_node_t *node = first; node != NULL; node = node->next) {
        generate_statement(node, node == last_value_statement, context);
    }
}

static ast_node_t *find_last_statement(ast_node_t *first) {
    ast_node_t *last = first;
    while (last != NULL && last->next != NULL) {
        last = last->next;
    }
    return last;
}

// Generates a function (the main function is the entry of the program)
static void generate_function(ast_node_t *node, codegen_context_t *context) {
    FILE *f = context->file;
    bool is_main = node->type == AST_NODE_MAIN;
    ast_node_t *first = node->fun_definition.body_block != NULL
                            ? node->fun_definition.body_block->block.first_statement
                            : NULL;
    ast_node_t *last = find_last_statement(first);

    context->is_in_main = is_main;
    context->is_unit_function = node->fun_definition.return_type == SYM_DATATYPE_UNIT ||
                                node->fun_definition.return_type == SYM_DATATYPE_UNKNOWN;

    if (is_main) {
        fprintf(f, "LABEL $main\n");
    } else {
        fprintf(f, "LABEL %s\n", node->fun_definition.name);
    }
    fprintf(f, "CREATEFRAME\n");
    fprintf(f, "PUSHFRAME\n");

    // define parameters and local variables
    for (symtable_fun_param_t *p = node->fun_definition.params; p != NULL; p = p->next) {
        fprintf(f, "DEFVAR LF@%s$%d\n", p->name, p->unique_id);
    }
    define_local_variables(first, context);

    // load parameters from the stack (the last one is on the top)
    int param_count = 0;
    for (symtable_fun_param_t *p = node->fun_definition.params; p != NULL; p = p->next) {
        param_count++;
    }
    for (int i = param_count; i > 0; i--) {
        symtable_fun_param_t *p = node->fun_definition.params;
        for (int j = 1; j < i; j++) {
            p = p->next;
        }
        fprintf(f, "POPS LF@%s$%d\n", p->name, p->unique_id);
    }

    // value of the last expression is the return value (unless the function returns Unit)
    bool keep_last = last != NULL && is_expression_node(last) && !context->is_unit_function;
    generate_statement_list(first, keep_last ? last : NULL, context);

    if (is_main) {
        fprintf(f, "EXIT int@0\n");
    } else {
        if (context->is_unit_function) {
            fprintf(f, "PUSHS nil@nil\n");
        }
        fprintf(f, "POPFRAME\n");
        fprintf(f, "RETURN\n");
    }
    context->is_in_main = false;
}

// main function for code generation, output_file == NULL means standard output
bool codegen(ast_node_t *ast_root, const char *output_file) {
    // create context including the output file
    codegen_context_t *context = create_codegen_context(output_file);
    if (context == NULL) {
        return false;
    }
    FILE *f = context->file;

    // startup code
    fprintf(f, ".IFJcode26\n");
    for (int i = 0; i < 6; i++) {
        fprintf(f, "DEFVAR GF@tmp%d\n", i);
    }

    bool has_main = false;
    if (ast_root != NULL && ast_root->type == AST_NODE_BLOCK) {
        for (ast_node_t *n = ast_root->block.first_statement; n != NULL; n = n->next) {
            if (n->type == AST_NODE_MAIN) {
                has_main = true;
            }
        }
    }
    if (has_main) {
        fprintf(f, "JUMP $main\n");
    }

    // functions
    if (ast_root != NULL && ast_root->type == AST_NODE_BLOCK) {
        for (ast_node_t *n = ast_root->block.first_statement; n != NULL; n = n->next) {
            if (n->type == AST_NODE_FUN_DEFINITION || n->type == AST_NODE_MAIN) {
                generate_function(n, context);
            }
        }
    }

    // end of file code
    if (context->is_overflow_check_needed) {
        generate_overflow_check(context);
    }
    fprintf(f, "LABEL $EXIT40\n");
    fprintf(f, "EXIT int@40\n");

    bool result = !context->failed;

    // cleanup
    destroy_codegen_context(context);
    return result;
}
