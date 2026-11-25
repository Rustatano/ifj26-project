/*
-------------------------------------------------------
Name of project: Translator IFJ26
Authors: xondrus00 (Sebastian Ondruska)
Creation date: 14.11.2026
File: parse_expression_common.c

Description: Common expression parser tables and helpers
-------------------------------------------------------
*/

#include "parse_expression_common.h"
#include "parse_expression_stack.h"

// Table with expression rules
const char parser_prec_table[PARSER_PREC_COUNT][PARSER_PREC_COUNT] = {
    /* unary       */ {'<', '>', '>', '>', '>', '<', '>', '<', '>'},
    /* mul div     */ {'<', '>', '>', '>', '>', '<', '>', '<', '>'},
    /* add sub     */ {'<', '<', '>', '>', '>', '<', '>', '<', '>'},
    /* relation    */ {'<', '<', '<', ' ', '>', '<', '>', '<', '>'},
    /* eq neq      */ {'<', '<', '<', '<', ' ', '<', '>', '<', '>'},
    /* l paren     */ {'<', '<', '<', '<', '<', '<', '=', '<', ' '},
    /* r paren     */ {' ', '>', '>', '>', '>', ' ', '>', ' ', '>'},
    /* operand     */ {' ', '>', '>', '>', '>', ' ', '>', ' ', '>'},
    /* dollar      */ {'<', '<', '<', '<', '<', '<', ' ', '<', ' '}};

// Function for remaping token to prec_table_index_t for precedence parsing
parser_prec_table_index_t parser_prec_table_token_to_prec_index(parser_context_t *context,
                                                                parser_prec_stack_t *stack) {
    token_type_t t = context->current_token.type;

    switch (t) {
    case TOKEN_PLUS:
    case TOKEN_MINUS:
        return parser_prec_stack_is_binary_after_top(stack) ? PARSER_PREC_ADD_SUB
                                                            : PARSER_PREC_UNARY;

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
        parser_prec_stack_item_t *top_term = parser_prec_stack_top_terminal(stack);
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

// Function for remaping token to ast_expression_operator_t for AST node creation
ast_expression_operator_t parser_remap_operator(token_type_t op) {
    switch (op) {
    case TOKEN_PLUS:
        return AST_EXPRESSION_OPERATOR_ADD;
    case TOKEN_MINUS:
        return AST_EXPRESSION_OPERATOR_SUB;
    case TOKEN_MUL:
        return AST_EXPRESSION_OPERATOR_MUL;
    case TOKEN_DIV:
        return AST_EXPRESSION_OPERATOR_DIV;
    case TOKEN_EQ:
        return AST_EXPRESSION_OPERATOR_EQUAL;
    case TOKEN_NEQ:
        return AST_EXPRESSION_OPERATOR_NOT_EQUAL;
    case TOKEN_LT:
        return AST_EXPRESSION_OPERATOR_SMALLER;
    case TOKEN_GT:
        return AST_EXPRESSION_OPERATOR_BIGGER;
    case TOKEN_LE:
        return AST_EXPRESSION_OPERATOR_SMALLER_EQUAL;
    case TOKEN_GE:
        return AST_EXPRESSION_OPERATOR_BIGGER_EQUAL;
    default:
        return AST_EXPRESSION_OPERATOR_ADD;
    }
}
