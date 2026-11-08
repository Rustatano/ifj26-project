

#include "ast.h"
#include "parser.c"
#include "scanner.h"
#include "symbol_table.h"
#include <stdbool.h>
#include <stdlib.h>

typedef enum {
    PARSER_PREC_UNARY = 0,
    PARSER_PREC_MUL_DIV,
    PARSER_PREC_ADD_SUB,
    PARSER_PREC_RELATION,
    PARSER_PREC_EQ_NEQ,
    PARSER_PREC_LPAREN,
    PARSER_PREC_RPAREN,
    PARSER_PREC_ID,
    PARSER_PREC_DOLLAR,
    PARSER_PREC_COUNT // Helper non terminal
} parser_prec_table_index_t;

typedef enum {
    PARSER_PREC_STACK_ITEM_TERMINAL,
    PARSER_PREC_STACK_ITEM_NONTERMINAL,
    PARSER_PREC_STACK_ITEM_HANDLE_MARK
} parser_prec_stack_item_type_t;

typedef struct parser_prec_stack_item {
    parser_prec_stack_item_type_t type;
    parser_prec_table_index_t index;
    token_type_t token_type;
    int source_code_line;
    ast_node_t *node;
    struct parser_prec_stack_item *next;
} parser_prec_stack_item_t;

typedef struct {
    parser_prec_stack_item_t *top;
} parser_prec_stack_t;

// < == shift
// > == reduce
// = == shift and read next token for )
// ' ' == syntax error in source code
//
// rows are the top terminal on the stack
// columns are the current input token
static const char parser_prec_table[PARSER_PREC_COUNT][PARSER_PREC_COUNT] = {
    /* unary       */ {'<', '>', '>', '>', '>', '<', '>', '<', '>'},
    /* mul div    */ {'<', '>', '>', '>', '>', '<', '>', '<', '>'},
    /* add sub   */ {'<', '<', '>', '>', '>', '<', '>', '<', '>'},
    /* relation    */ {'<', '<', '<', '>', '>', '<', '>', '<', '>'},
    /* eq neq     */ {'<', '<', '<', '<', '>', '<', '>', '<', '>'},
    /* l paren     */ {'<', '<', '<', '<', '<', '<', '=', '<', ' '},
    /* r paren     */ {' ', '>', '>', '>', '>', ' ', '>', ' ', '>'},
    /* operand   */ {' ', '>', '>', '>', '>', ' ', '>', ' ', '>'},
    /* dollar        */ {'<', '<', '<', '<', '<', '<', ' ', '<', ' '}};

ast_expres_operator_t remap_operator(token_type_t operator) {
    switch (operator) {
    case TOKEN_PLUS:
        return AST_EXPRES_OPERATOR_ADD;
    case TOKEN_MINUS:
        return AST_EXPRES_OPERATOR_SUB;
    case TOKEN_MUL:
        return AST_EXPRES_OPERATOR_MUL;
    case TOKEN_DIV:
        return AST_EXPRES_OPERATOR_DIV;
    case TOKEN_EQ:
        return AST_EXPRES_OPERATOR_EQUAL;
    case TOKEN_NEQ:
        return AST_EXPRES_OPERATOR_NOT_EQUAL;
    case TOKEN_LT:
        return AST_EXPRES_OPERATOR_SMALLER;
    case TOKEN_GT:
        return AST_EXPRES_OPERATOR_BIGGER;
    case TOKEN_LE:
        return AST_EXPRES_OPERATOR_SMALLER_EQUAL;
    case TOKEN_GE:
        return AST_EXPRES_OPERATOR_BIGGER_EQUAL;
    default:
        return AST_EXPRES_OPERATOR_ADD;
    }
}

void prec_stack_init(parser_prec_stack_t *stack) { stack->top = NULL; }

bool prec_stack_push(parser_prec_stack_t *stack, parser_prec_stack_item_type_t item_type,
                     parser_prec_table_index_t prec_idx, token_type_t token_type, int line,
                     ast_node_t *node) {
    parser_prec_stack_item_t *item = calloc(1, sizeof(parser_prec_stack_item_t));
    if (item == NULL) {
        return false;
    }
    item->type = item_type;
    item->index = prec_idx;
    item->token_type = token_type;
    item->source_code_line = line;
    item->node = node;
    item->next = stack->top;
    stack->top = item;
    return true;
}

parser_prec_stack_item_t *prec_stack_top_terminal(parser_prec_stack_t *stack) {
    parser_prec_stack_item_t *current = stack->top;
    while (current != NULL) {
        if (current->type == PARSER_PREC_STACK_ITEM_TERMINAL) {
            return current;
        }
        current = current->next;
    }
    return NULL;
}

bool prec_stack_insert_handle_mark(parser_prec_stack_t *stack) {
    parser_prec_stack_item_t *mark = calloc(1, sizeof(parser_prec_stack_item_t));
    if (mark == NULL) {
        return false;
    }
    mark->type = PARSER_PREC_STACK_ITEM_HANDLE_MARK;

    if (stack->top == NULL || stack->top->type == PARSER_PREC_STACK_ITEM_TERMINAL) {
        mark->next = stack->top;
        stack->top = mark;
        return true;
    }

    parser_prec_stack_item_t *previous = stack->top;
    while (previous->next != NULL && previous->next->type != PARSER_PREC_STACK_ITEM_TERMINAL) {
        previous = previous->next;
    }
    mark->next = previous->next;
    previous->next = mark;
    return true;
}

void prec_stack_pop(parser_prec_stack_t *stack) {
    if (!stack->top) {
        return;
    }
    parser_prec_stack_item_t *tmp = stack->top;
    stack->top = tmp->next;
    free(tmp);
}

void prec_stack_dispose(parser_prec_stack_t *stack) {
    while (stack->top != NULL) {
        parser_prec_stack_item_t *tmp = stack->top;
        stack->top = tmp->next;
        if (tmp->node != NULL) {
            ast_dispose(&tmp->node);
        }
        free(tmp);
    }
}

bool prec_stack_is_reduced(parser_prec_stack_t *stack) {
    if (stack->top == NULL || stack->top->type != PARSER_PREC_STACK_ITEM_NONTERMINAL) {
        return false;
    }
    parser_prec_stack_item_t *below = stack->top->next;
    if (below == NULL || below->type != PARSER_PREC_STACK_ITEM_TERMINAL ||
        below->index != PARSER_PREC_DOLLAR) {
        return false;
    }
    return below->next == NULL;
}

bool prec_stack_is_binary_after_top(parser_prec_stack_t *stack) {
    if (stack->top == NULL) {
        return false;
    }
    if (stack->top->type == PARSER_PREC_STACK_ITEM_NONTERMINAL) {
        return true;
    }
    if (stack->top->type == PARSER_PREC_STACK_ITEM_TERMINAL &&
        (stack->top->index == PARSER_PREC_ID || stack->top->index == PARSER_PREC_RPAREN)) {
        return true;
    }
    return false;
}

parser_prec_table_index_t prec_table_token_to_prec_index(parser_context_t *context,
                                                         parser_prec_stack_t *stack) {
    token_type_t t = context->current_token.type;

    switch (t) {
    case TOKEN_PLUS:
    case TOKEN_MINUS:
        return prec_stack_is_binary_after_top(stack) ? PARSER_PREC_ADD_SUB : PARSER_PREC_UNARY;

    case TOKEN_MUL:
    case TOKEN_DIV:
        return PARSER_PREC_MUL_DIV;

    case TOKEN_LT:
    case TOKEN_GT:
    case TOKEN_LE:
    case TOKEN_GE:
        return PARSER_PREC_RELATION;

    case TOKEN_EQ:
    case TOKEN_NEQ:
        return PARSER_PREC_EQ_NEQ;

    case TOKEN_LPAREN:
        return PARSER_PREC_LPAREN;

    case TOKEN_RPAREN: {
        parser_prec_stack_item_t *top_term = prec_stack_top_terminal(stack);
        if (top_term != NULL && top_term->index == PARSER_PREC_DOLLAR &&
            stack->top->type == PARSER_PREC_STACK_ITEM_NONTERMINAL) {
            return PARSER_PREC_DOLLAR;
        }
        return PARSER_PREC_RPAREN;
    }

    case TOKEN_INT:
    case TOKEN_DOUBLE:
    case TOKEN_STRING:
    case TOKEN_ID:
        return PARSER_PREC_ID;

    default:
        return PARSER_PREC_DOLLAR;
    }
}

ast_node_t *parse_operand(parser_context_t *context) {
    token_t token = context->current_token;

    switch (token.type) {
    case TOKEN_INT:
        load_token(context);
        return ast_const_int_create(token.attr.int_val, token.line);

    case TOKEN_DOUBLE:
        load_token(context);
        return ast_const_double_create(token.attr.double_val, token.line);

    case TOKEN_STRING:
        load_token(context);
        return ast_const_string_create(token.attr.str_val, token.line);

    case TOKEN_ID: {
        char *name = token.attr.str_val;
        int line = token.line;
        load_token(context);

        if (check_token(context, TOKEN_LPAREN)) {
            load_token(context);

            ast_node_t *params = NULL;
            ast_node_t *last_param = NULL;
            int params_count = 0;

            if (!check_token(context, TOKEN_RPAREN)) {
                while (true) {
                    ast_node_t *argument = parse_expresion(context);
                    if (argument == NULL) {
                        ast_dispose(&params);
                        return NULL;
                    }
                    if (params == NULL) {
                        params = argument;
                    } else {
                        last_param->next = argument;
                    }
                    last_param = argument;
                    params_count++;

                    if (check_token(context, TOKEN_RPAREN)) {
                        break;
                    }
                    if (!expect_token(context, TOKEN_COMMA,
                                      "Expected ',' or ')' in function call arguments")) {
                        ast_dispose(&params);
                        return NULL;
                    }
                    load_token(context);
                }
            }
            load_token(context);
            return ast_expres_fun_call_create(name, params, params_count, line);
        }
        return ast_expres_var_create(name, 0, SYM_DATATYPE_ID, line);
    }

    default:
        context->error_line = token.line;
        context->error_message = "Expected operand in expression";
        return NULL;
    }
}

bool prec_table_reduce(parser_context_t *context, parser_prec_stack_t *stack) {
    int handle_length = 0;
    parser_prec_stack_item_t *current = stack->top;
    while (current != NULL && current->type != PARSER_PREC_STACK_ITEM_HANDLE_MARK) {
        handle_length++;
        current = current->next;
    }

    if (current == NULL) {
        context->error_line = context->current_token.line;
        context->error_message = "Syntax error in expression: missing handle mark";
        return false;
    }

    if (handle_length == 1) {
        parser_prec_stack_item_t *s1 = stack->top;
        if (s1->type == PARSER_PREC_STACK_ITEM_TERMINAL && s1->index == PARSER_PREC_ID) {
            ast_node_t *node = s1->node;
            int line = s1->source_code_line;
            s1->node = NULL;

            prec_stack_pop(stack);
            prec_stack_pop(stack);
            return prec_stack_push(stack, PARSER_PREC_STACK_ITEM_NONTERMINAL, PARSER_PREC_COUNT, 0,
                                   line, node);
        }
    }
    // E -> unary E
    else if (handle_length == 2) {
        parser_prec_stack_item_t *right_item = stack->top;
        parser_prec_stack_item_t *unary_operand = stack->top->next;

        if (unary_operand->type == PARSER_PREC_STACK_ITEM_TERMINAL &&
            unary_operand->index == PARSER_PREC_UNARY &&
            right_item->type == PARSER_PREC_STACK_ITEM_NONTERMINAL) {
            ast_node_t *operand = right_item->node;
            int line = unary_operand->source_code_line;
            token_type_t op_type = unary_operand->token_type;
            right_item->node = NULL;

            prec_stack_pop(stack); // Pop E
            prec_stack_pop(stack); // Pop operand
            prec_stack_pop(stack); // Pop <

            ast_node_t *result_node = operand;
            if (op_type == TOKEN_MINUS) {
                ast_node_t *zero = ast_const_int_create(0, line);
                result_node =
                    ast_expres_operation_create(AST_EXPRES_OPERATOR_SUB, zero, operand, line);
                if (result_node == NULL) {
                    ast_dispose(&zero);
                    ast_dispose(&operand);
                    return false;
                }
            }
            return prec_stack_push(stack, PARSER_PREC_STACK_ITEM_NONTERMINAL, PARSER_PREC_COUNT, 0,
                                   line, result_node);
        }
    }
    // E => E binary E or E => ( E )
    else if (handle_length == 3) {
        parser_prec_stack_item_t *right_item = stack->top;
        parser_prec_stack_item_t *binary_operand = stack->top->next;
        parser_prec_stack_item_t *left_item = stack->top->next->next;

        //  E => ( E )
        if (left_item->type == PARSER_PREC_STACK_ITEM_TERMINAL &&
            left_item->index == PARSER_PREC_LPAREN &&
            binary_operand->type == PARSER_PREC_STACK_ITEM_NONTERMINAL &&
            right_item->type == PARSER_PREC_STACK_ITEM_TERMINAL &&
            right_item->index == PARSER_PREC_RPAREN) {
            ast_node_t *inner_node = binary_operand->node;
            int line = left_item->source_code_line;
            binary_operand->node = NULL;

            prec_stack_pop(stack); // Pop )
            prec_stack_pop(stack); // Pop E
            prec_stack_pop(stack); // Pop (
            prec_stack_pop(stack); // Pop <
            return prec_stack_push(stack, PARSER_PREC_STACK_ITEM_NONTERMINAL, PARSER_PREC_COUNT, 0,
                                   line, inner_node);
        }

        //  E => E binary E
        if (left_item->type == PARSER_PREC_STACK_ITEM_NONTERMINAL &&
            binary_operand->type == PARSER_PREC_STACK_ITEM_TERMINAL &&
            right_item->type == PARSER_PREC_STACK_ITEM_NONTERMINAL) {
            ast_node_t *left = left_item->node;
            ast_node_t *right = right_item->node;
            int line = binary_operand->source_code_line;
            token_type_t op_token = binary_operand->token_type;
            left_item->node = NULL;
            right_item->node = NULL;

            prec_stack_pop(stack); // Pop E
            prec_stack_pop(stack); // Pop operand
            prec_stack_pop(stack); // Pop  E
            prec_stack_pop(stack); // Pop <

            ast_node_t *op_node =
                ast_expres_operation_create(remap_operator(op_token), left, right, line);
            if (!op_node) {
                ast_dispose(&left);
                ast_dispose(&right);
                return false;
            }
            return prec_stack_push(stack, PARSER_PREC_STACK_ITEM_NONTERMINAL, PARSER_PREC_COUNT, 0,
                                   line, op_node);
        }
    }

    context->error_line = context->current_token.line;
    context->error_message = "Syntax error: no matching rule for expression reduction";
    return false;
}

ast_node_t *parse_expresion(parser_context_t *context) {
    parser_prec_stack_t stack;
    prec_stack_init(&stack);

    if (!prec_stack_push(&stack, PARSER_PREC_STACK_ITEM_TERMINAL, PARSER_PREC_DOLLAR, TOKEN_EOF,
                         context->current_token.line, NULL)) {
        context->error_line = context->current_token.line;
        context->error_message = "Memory allocation failed for precedence stack";
        return NULL;
    }

    while (true) {
        parser_prec_stack_item_t *top_terminal = prec_stack_top_terminal(&stack);
        parser_prec_table_index_t current_prec_index =
            prec_table_token_to_prec_index(context, &stack);

        if (current_prec_index == PARSER_PREC_DOLLAR && prec_stack_is_reduced(&stack)) {
            ast_node_t *result = stack.top->node;
            stack.top->node = NULL;
            prec_stack_dispose(&stack);
            return result;
        }

        char relation = parser_prec_table[top_terminal->index][current_prec_index];

        if (relation == '=') {
            if (!prec_stack_push(&stack, PARSER_PREC_STACK_ITEM_TERMINAL, current_prec_index,
                                 context->current_token.type, context->current_token.line, NULL)) {
                prec_stack_dispose(&stack);
                return NULL;
            }
            load_token(context);
        } else if (relation == '<') {
            if (!prec_stack_insert_handle_mark(&stack)) {
                prec_stack_dispose(&stack);
                return NULL;
            }

            if (current_prec_index == PARSER_PREC_ID) {
                int line = context->current_token.line;
                token_type_t tok_type = context->current_token.type;
                ast_node_t *operand_node = parse_operand(context);
                if (!operand_node) {
                    prec_stack_dispose(&stack);
                    return NULL;
                }
                if (!prec_stack_push(&stack, PARSER_PREC_STACK_ITEM_TERMINAL, PARSER_PREC_ID,
                                     tok_type, line, operand_node)) {
                    ast_dispose(&operand_node);
                    prec_stack_dispose(&stack);
                    return NULL;
                }
            } else {
                if (!prec_stack_push(&stack, PARSER_PREC_STACK_ITEM_TERMINAL, current_prec_index,
                                     context->current_token.type, context->current_token.line,
                                     NULL)) {
                    prec_stack_dispose(&stack);
                    return NULL;
                }
                load_token(context);
            }
        } else if (relation == '>') {
            if (!prec_table_reduce(context, &stack)) {
                prec_stack_dispose(&stack);
                return NULL;
            }
        } else {
            context->error_line = context->current_token.line;
            context->error_message = "Syntax error in expression";
            prec_stack_dispose(&stack);
            return NULL;
        }
    }
}
