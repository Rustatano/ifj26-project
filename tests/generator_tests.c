/*
-------------------------------------------------------
Name of project: Translator IFJ26
Authors: Zobal, Tajovský, Ondruška, Svoboda
Creation date: 11.10.2026
File: generator_tests.c

Description: Unit tests for generator of IFJcode26.

-------------------------------------------------------
*/

#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <cmocka.h> // must be under std includes

#include "codegen.h"

#define CODEGEN_TEST_OUTPUT "/tmp/ifj26_codegen_test.out"
#define CODEGEN_RUNTIME_OUTPUT "/tmp/ifj26_codegen_runtime.out"
#define CODEGEN_RUNTIME_INPUT "/tmp/ifj26_codegen_runtime.in"
#define IC26INT "./ic26int-linux-x86_64"

static ast_node_t *create_test_node(ast_node_type_t type) {
    ast_node_t *node = calloc(1, sizeof(*node));
    assert_non_null(node);
    node->type = type;
    node->source_code_line = 1;
    return node;
}

static ast_node_t *create_test_main(void) { return create_test_node(AST_NODE_MAIN); }

static ast_node_t *create_test_block(ast_node_t *first_statement) {
    ast_node_t *block = create_test_node(AST_NODE_BLOCK);
    block->block.first_statement = first_statement;
    return block;
}

static ast_node_t *create_test_program(ast_node_t *first_function) {
    return create_test_block(first_function);
}

static ast_node_t *create_test_int(int value) {
    ast_node_t *node = create_test_node(AST_NODE_CONST_INT);
    node->int_value = value;
    node->result_type = SYM_DATATYPE_INT;
    return node;
}

static ast_node_t *create_test_string(char *value) {
    ast_node_t *node = create_test_node(AST_NODE_CONST_STRING);
    node->str_value = value;
    node->result_type = SYM_DATATYPE_STRING;
    return node;
}

static ast_node_t *create_test_operation(ast_expression_operator_t op, ast_node_t *left,
                                         ast_node_t *right, symtable_datatype_t result_type) {
    ast_node_t *node = create_test_node(AST_NODE_EXPRESSION_OPERATOR);
    node->expression_operator.op = op;
    node->expression_operator.left_side = left;
    node->expression_operator.right_side = right;
    node->result_type = result_type;
    return node;
}

static ast_node_t *create_test_call(char *name, ast_node_t *params,
                                    symtable_datatype_t result_type) {
    ast_node_t *node = create_test_node(AST_NODE_EXPRESSION_FUN_CALL);
    node->expression_fun_call.fun_name = name;
    node->expression_fun_call.params = params;
    node->expression_fun_call.params_count = params == NULL ? 0 : 1;
    node->result_type = result_type;
    return node;
}

static ast_node_t *create_test_var(char *name, int unique_id, symtable_datatype_t result_type) {
    ast_node_t *node = create_test_node(AST_NODE_EXPRESSION_VAR);
    node->expression_var.name = name;
    node->expression_var.unique_id = unique_id;
    node->result_type = result_type;
    return node;
}

static ast_node_t *create_test_return(ast_node_t *expression) {
    ast_node_t *node = create_test_node(AST_NODE_STATEMENT_RETURN);
    node->return_statement.expression = expression;
    return node;
}

static ast_node_t *create_test_if(ast_node_t *condition, ast_node_t *if_block,
                                  ast_node_t *else_block) {
    ast_node_t *node = create_test_node(AST_NODE_STATEMENT_IF);
    node->if_statement.condition = condition;
    node->if_statement.if_block = if_block;
    node->if_statement.else_block = else_block;
    return node;
}

static ast_node_t *create_test_while(ast_node_t *condition, ast_node_t *body_block) {
    ast_node_t *node = create_test_node(AST_NODE_STATEMENT_WHILE);
    node->while_statement.condition = condition;
    node->while_statement.body_block = body_block;
    return node;
}

static ast_node_t *create_test_declaration(char *name, int unique_id, symtable_datatype_t type,
                                           ast_node_t *initializer) {
    ast_node_t *node = create_test_node(AST_NODE_VAR_DECLARATION);
    node->var_declaration.name = name;
    node->var_declaration.unique_id = unique_id;
    node->var_declaration.var_type = type;
    node->var_declaration.init_expression = initializer;
    return node;
}

static ast_node_t *create_test_assignment(char *name, int unique_id, ast_node_t *expression) {
    ast_node_t *node = create_test_node(AST_NODE_VALUE_ASSIGNMENT);
    node->assign_statement.target_var_name = name;
    node->assign_statement.unique_id = unique_id;
    node->assign_statement.expression = expression;
    return node;
}

static symtable_fun_param_t *create_test_param(char *name, int unique_id,
                                               symtable_datatype_t type) {
    symtable_fun_param_t *param = calloc(1, sizeof(*param));
    assert_non_null(param);
    param->name = name;
    param->unique_id = unique_id;
    param->type = type;
    return param;
}

static ast_node_t *create_test_function(ast_node_type_t type, char *name,
                                        symtable_fun_param_t *params,
                                        symtable_datatype_t return_type, ast_node_t *body_block) {
    ast_node_t *node = create_test_node(type);
    node->fun_definition.name = name;
    node->fun_definition.params = params;
    node->fun_definition.return_type = return_type;
    node->fun_definition.body_block = body_block;
    return node;
}

static void free_test_params(symtable_fun_param_t *params) {
    while (params != NULL) {
        symtable_fun_param_t *next = params->next;
        free(params);
        params = next;
    }
}

static void free_test_ast(ast_node_t *node) {
    if (node == NULL) {
        return;
    }

    free_test_ast(node->next);
    switch (node->type) {
    case AST_NODE_MAIN:
    case AST_NODE_BLOCK:
        free_test_ast(node->block.first_statement);
        break;
    case AST_NODE_VAR_DECLARATION:
        free_test_ast(node->var_declaration.init_expression);
        break;
    case AST_NODE_VALUE_ASSIGNMENT:
        free_test_ast(node->assign_statement.expression);
        break;
    case AST_NODE_STATEMENT_IF:
        free_test_ast(node->if_statement.condition);
        free_test_ast(node->if_statement.if_block);
        free_test_ast(node->if_statement.else_block);
        break;
    case AST_NODE_STATEMENT_WHILE:
        free_test_ast(node->while_statement.condition);
        free_test_ast(node->while_statement.body_block);
        break;
    case AST_NODE_STATEMENT_RETURN:
        free_test_ast(node->return_statement.expression);
        break;
    case AST_NODE_FUN_DEFINITION:
        free_test_params(node->fun_definition.params);
        free_test_ast(node->fun_definition.body_block);
        break;
    case AST_NODE_EXPRESSION_FUN_CALL:
        free_test_ast(node->expression_fun_call.params);
        break;
    case AST_NODE_EXPRESSION_OPERATOR:
        free_test_ast(node->expression_operator.left_side);
        free_test_ast(node->expression_operator.right_side);
        break;
    default:
        break;
    }
    free(node);
}

static char *run_generated_program(const char *input) {
    FILE *input_file = fopen(CODEGEN_RUNTIME_INPUT, "w");
    assert_non_null(input_file);
    if (input != NULL) {
        assert_int_equal(fputs(input, input_file), 0);
    }
    assert_int_equal(fclose(input_file), 0);

    char command[512];
    int command_result = snprintf(command, sizeof(command),
                                  IC26INT " " CODEGEN_TEST_OUTPUT " < " CODEGEN_RUNTIME_INPUT
                                          " > " CODEGEN_RUNTIME_OUTPUT);
    assert_true(command_result > 0 && (size_t)command_result < sizeof(command));
    assert_int_equal(system(command), 0);

    FILE *file = fopen(CODEGEN_RUNTIME_OUTPUT, "r");
    assert_non_null(file);
    assert_int_equal(fseek(file, 0, SEEK_END), 0);
    long file_size = ftell(file);
    assert_true(file_size >= 0);
    rewind(file);

    char *output = calloc((size_t)file_size + 1, sizeof(*output));
    assert_non_null(output);
    assert_int_equal(fread(output, 1, (size_t)file_size, file), (size_t)file_size);
    assert_int_equal(fclose(file), 0);
    return output;
}

void codegen_program_and_constants_test(void **state) {
    (void)state;
    ast_node_t *main_node = create_test_main();
    ast_node_t *program;
    ast_node_t *int_node = create_test_int(42);
    ast_node_t *double_node = create_test_node(AST_NODE_CONST_DOUBLE);
    double_node->double_value = 1.5;
    double_node->result_type = SYM_DATATYPE_DOUBLE;
    ast_node_t *string_node = create_test_node(AST_NODE_CONST_STRING);
    string_node->str_value = "hello";
    string_node->result_type = SYM_DATATYPE_STRING;
    ast_node_t *print_int = create_test_call("print", int_node, SYM_DATATYPE_UNIT);
    ast_node_t *print_double = create_test_call("print", double_node, SYM_DATATYPE_UNIT);
    ast_node_t *print_string = create_test_call("print", string_node, SYM_DATATYPE_UNIT);
    main_node->block.first_statement = create_test_block(print_int);
    print_int->next = print_double;
    print_double->next = print_string;
    program = create_test_program(main_node);

    bool result = codegen(program, CODEGEN_TEST_OUTPUT);
    char *output = run_generated_program(NULL);

    assert_true(result);
    assert_string_equal(output, "420x1.8p+0hello");

    free(output);
    free_test_ast(program);
    remove(CODEGEN_TEST_OUTPUT);
    remove(CODEGEN_RUNTIME_OUTPUT);
    remove(CODEGEN_RUNTIME_INPUT);
}

void codegen_variable_declaration_and_assignment_test(void **state) {
    (void)state;
    ast_node_t *main_node = create_test_main();
    ast_node_t *program;
    ast_node_t *declaration = create_test_node(AST_NODE_VAR_DECLARATION);
    declaration->var_declaration.name = "value";
    declaration->var_declaration.unique_id = 7;
    declaration->var_declaration.var_type = SYM_DATATYPE_INT;
    declaration->var_declaration.init_expression = create_test_int(10);
    ast_node_t *assignment = create_test_node(AST_NODE_VALUE_ASSIGNMENT);
    assignment->assign_statement.target_var_name = "value";
    assignment->assign_statement.unique_id = 7;
    assignment->assign_statement.expression = create_test_int(20);
    ast_node_t *print_value =
        create_test_call("print", create_test_var("value", 7, SYM_DATATYPE_INT), SYM_DATATYPE_UNIT);
    main_node->block.first_statement = create_test_block(declaration);
    declaration->next = assignment;
    assignment->next = print_value;
    program = create_test_program(main_node);

    bool result = codegen(program, CODEGEN_TEST_OUTPUT);
    char *output = run_generated_program(NULL);

    assert_true(result);
    assert_string_equal(output, "20");

    free(output);
    free_test_ast(program);
    remove(CODEGEN_TEST_OUTPUT);
    remove(CODEGEN_RUNTIME_OUTPUT);
    remove(CODEGEN_RUNTIME_INPUT);
}

void codegen_integer_arithmetic_test(void **state) {
    (void)state;
    ast_node_t *main_node = create_test_main();
    ast_node_t *program;
    ast_node_t *operation = create_test_node(AST_NODE_EXPRESSION_OPERATOR);
    operation->expression_operator.op = AST_EXPRESSION_OPERATOR_ADD;
    operation->expression_operator.left_side = create_test_int(1);
    operation->expression_operator.right_side = create_test_int(2);
    operation->result_type = SYM_DATATYPE_INT;
    ast_node_t *print_operation = create_test_call("print", operation, SYM_DATATYPE_UNIT);
    main_node->block.first_statement = create_test_block(print_operation);
    program = create_test_program(main_node);

    bool result = codegen(program, CODEGEN_TEST_OUTPUT);
    char *output = run_generated_program(NULL);

    assert_true(result);
    assert_string_equal(output, "3");

    free(output);
    free_test_ast(program);
    remove(CODEGEN_TEST_OUTPUT);
    remove(CODEGEN_RUNTIME_OUTPUT);
    remove(CODEGEN_RUNTIME_INPUT);
}

void codegen_factorial_program_test(void **state) {
    (void)state;

    symtable_fun_param_t *iterative_param = create_test_param("n", 1, SYM_DATATYPE_INT);
    ast_node_t *iterative_result =
        create_test_declaration("result", 2, SYM_DATATYPE_INT, create_test_int(1));
    ast_node_t *iterative_i = create_test_declaration("i", 3, SYM_DATATYPE_INT, create_test_int(2));
    ast_node_t *iterative_loop_body = create_test_assignment(
        "result", 2,
        create_test_operation(AST_EXPRESSION_OPERATOR_MUL,
                              create_test_var("result", 2, SYM_DATATYPE_INT),
                              create_test_var("i", 3, SYM_DATATYPE_INT), SYM_DATATYPE_INT));
    iterative_loop_body->next =
        create_test_assignment("i", 3,
                               create_test_operation(AST_EXPRESSION_OPERATOR_ADD,
                                                     create_test_var("i", 3, SYM_DATATYPE_INT),
                                                     create_test_int(1), SYM_DATATYPE_INT));
    ast_node_t *iterative_while = create_test_while(
        create_test_operation(AST_EXPRESSION_OPERATOR_SMALLER_EQUAL,
                              create_test_var("i", 3, SYM_DATATYPE_INT),
                              create_test_var("n", 1, SYM_DATATYPE_INT), SYM_DATATYPE_INT),
        create_test_block(iterative_loop_body));
    iterative_result->next = iterative_i;
    iterative_i->next = iterative_while;
    iterative_while->next = create_test_var("result", 2, SYM_DATATYPE_INT);
    ast_node_t *iterative_function =
        create_test_function(AST_NODE_FUN_DEFINITION, "factorialIterative", iterative_param,
                             SYM_DATATYPE_INT, create_test_block(iterative_result));

    symtable_fun_param_t *recursive_param = create_test_param("n", 4, SYM_DATATYPE_INT);
    ast_node_t *recursive_return_one = create_test_return(create_test_int(1));
    ast_node_t *recursive_previous =
        create_test_declaration("previous", 5, SYM_DATATYPE_INT,
                                create_test_operation(AST_EXPRESSION_OPERATOR_SUB,
                                                      create_test_var("n", 4, SYM_DATATYPE_INT),
                                                      create_test_int(1), SYM_DATATYPE_INT));
    ast_node_t *recursive_smaller = create_test_declaration(
        "smaller", 6, SYM_DATATYPE_INT,
        create_test_call("factorialRecursive", create_test_var("previous", 5, SYM_DATATYPE_INT),
                         SYM_DATATYPE_INT));
    ast_node_t *recursive_return = create_test_return(create_test_operation(
        AST_EXPRESSION_OPERATOR_MUL, create_test_var("n", 4, SYM_DATATYPE_INT),
        create_test_var("smaller", 6, SYM_DATATYPE_INT), SYM_DATATYPE_INT));
    recursive_previous->next = recursive_smaller;
    recursive_smaller->next = recursive_return;
    ast_node_t *recursive_if = create_test_if(
        create_test_operation(AST_EXPRESSION_OPERATOR_SMALLER_EQUAL,
                              create_test_var("n", 4, SYM_DATATYPE_INT), create_test_int(1),
                              SYM_DATATYPE_INT),
        create_test_block(recursive_return_one), create_test_block(recursive_previous));
    symtable_fun_param_t *recursive_params = recursive_param;
    ast_node_t *recursive_function =
        create_test_function(AST_NODE_FUN_DEFINITION, "factorialRecursive", recursive_params,
                             SYM_DATATYPE_INT, create_test_block(recursive_if));

    ast_node_t *main_input = create_test_declaration(
        "n", 7, SYM_DATATYPE_INT, create_test_call("readInt", NULL, SYM_DATATYPE_INT));
    ast_node_t *invalid_message = create_test_call(
        "print", create_test_string("Use an integer from 0 to 12.\n"), SYM_DATATYPE_UNIT);
    ast_node_t *iterative_call = create_test_call(
        "factorialIterative", create_test_var("n", 7, SYM_DATATYPE_INT), SYM_DATATYPE_INT);
    ast_node_t *recursive_call = create_test_call(
        "factorialRecursive", create_test_var("n", 7, SYM_DATATYPE_INT), SYM_DATATYPE_INT);
    ast_node_t *main_valid_body =
        create_test_declaration("iterative", 8, SYM_DATATYPE_INT, iterative_call);
    main_valid_body->next =
        create_test_declaration("recursive", 9, SYM_DATATYPE_INT, recursive_call);
    main_valid_body->next->next =
        create_test_call("print", create_test_string("iterative: "), SYM_DATATYPE_UNIT);
    main_valid_body->next->next->next = create_test_call(
        "print", create_test_var("iterative", 8, SYM_DATATYPE_INT), SYM_DATATYPE_UNIT);
    main_valid_body->next->next->next->next =
        create_test_call("print", create_test_string("\nrecursive: "), SYM_DATATYPE_UNIT);
    main_valid_body->next->next->next->next->next = create_test_call(
        "print", create_test_var("recursive", 9, SYM_DATATYPE_INT), SYM_DATATYPE_UNIT);
    main_valid_body->next->next->next->next->next->next =
        create_test_call("print", create_test_string("\n"), SYM_DATATYPE_UNIT);

    ast_node_t *main_node = create_test_function(AST_NODE_MAIN, "factorialExample", NULL,
                                                 SYM_DATATYPE_UNIT, create_test_block(main_input));
    main_input->next = create_test_if(
        create_test_operation(AST_EXPRESSION_OPERATOR_SMALLER,
                              create_test_var("n", 7, SYM_DATATYPE_INT), create_test_int(0),
                              SYM_DATATYPE_INT),
        create_test_block(invalid_message),
        create_test_block(create_test_if(
            create_test_operation(AST_EXPRESSION_OPERATOR_BIGGER,
                                  create_test_var("n", 7, SYM_DATATYPE_INT), create_test_int(12),
                                  SYM_DATATYPE_INT),
            create_test_block(create_test_call(
                "print", create_test_string("Use an integer from 0 to 12.\n"), SYM_DATATYPE_UNIT)),
            create_test_block(main_valid_body))));

    iterative_function->next = recursive_function;
    recursive_function->next = main_node;
    ast_node_t *program = create_test_program(iterative_function);

    bool result = codegen(program, CODEGEN_TEST_OUTPUT);
    char *output = run_generated_program("5\n");

    assert_true(result);
    assert_string_equal(output, "iterative: 120\nrecursive: 120\n");

    free(output);
    free_test_ast(program);
    // remove(CODEGEN_TEST_OUTPUT);
    remove(CODEGEN_RUNTIME_OUTPUT);
    remove(CODEGEN_RUNTIME_INPUT);
}
