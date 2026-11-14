#ifndef PARSE_EXPRESSION_COMMON_H
#define PARSE_EXPRESSION_COMMON_H

#include "parser_common.h"

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
extern const char parser_prec_table[PARSER_PREC_COUNT][PARSER_PREC_COUNT];

parser_prec_table_index_t parser_prec_table_token_to_prec_index(parser_context_t *context,
                                                                parser_prec_stack_t *stack);
bool parser_prec_table_reduce(parser_context_t *context, parser_prec_stack_t *stack);
ast_expression_operator_t parser_remap_operator(token_type_t op);
#endif
