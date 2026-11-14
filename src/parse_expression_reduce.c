
#include "parse_expression_reduce.h"

static bool reduce_operand(parser_context_t *context, parser_prec_stack_t *stack) {
    parser_prec_stack_item_t *item = stack->top;
    if (item->type != PARSER_PREC_STACK_ITEM_TERMINAL || item->index != PARSER_PREC_ID) {
        return false;
    }

    ast_node_t *node = item->node;
    int line = item->source_code_line;
    item->node = NULL;
    parser_prec_stack_pop(stack);
    parser_prec_stack_pop(stack);

    if (!parser_prec_stack_push(stack, PARSER_PREC_STACK_ITEM_NONTERMINAL, PARSER_PREC_COUNT, 0,
                                line, node)) {
        ast_dispose(&node);
        parser_fail_internal(context, line);
        return false;
    }
    return true;
}

static bool reduce_unit_literal(parser_context_t *context, parser_prec_stack_t *stack) {
    parser_prec_stack_item_t *right_item = stack->top;
    parser_prec_stack_item_t *left_item = right_item->next;
    if (left_item->type != PARSER_PREC_STACK_ITEM_TERMINAL ||
        left_item->index != PARSER_PREC_LPAREN ||
        right_item->type != PARSER_PREC_STACK_ITEM_TERMINAL ||
        right_item->index != PARSER_PREC_RPAREN) {
        return false;
    }

    int line = left_item->source_code_line;
    parser_prec_stack_pop(stack);
    parser_prec_stack_pop(stack);
    parser_prec_stack_pop(stack);

    ast_node_t *unit_node = ast_const_unit_create(line);
    if (unit_node == NULL) {
        parser_fail_internal(context, line);
        return false;
    }
    if (!parser_prec_stack_push(stack, PARSER_PREC_STACK_ITEM_NONTERMINAL, PARSER_PREC_COUNT, 0,
                                line, unit_node)) {
        ast_dispose(&unit_node);
        parser_fail_internal(context, line);
        return false;
    }
    return true;
}

static bool reduce_unary_expression(parser_context_t *context, parser_prec_stack_t *stack) {
    parser_prec_stack_item_t *right_item = stack->top;
    parser_prec_stack_item_t *operator_item = right_item->next;
    if (operator_item->type != PARSER_PREC_STACK_ITEM_TERMINAL ||
        operator_item->index != PARSER_PREC_UNARY ||
        right_item->type != PARSER_PREC_STACK_ITEM_NONTERMINAL) {
        return false;
    }

    ast_node_t *operand = right_item->node;
    int line = operator_item->source_code_line;
    ast_expression_operator_t operator= operator_item->token_type == TOKEN_PLUS
        ? AST_EXPRESSION_OPERATOR_UNARY_PLUS
        : AST_EXPRESSION_OPERATOR_UNARY_MINUS;
    right_item->node = NULL;
    parser_prec_stack_pop(stack);
    parser_prec_stack_pop(stack);
    parser_prec_stack_pop(stack);

    ast_node_t *result = ast_expression_operation_create(operator, NULL, operand, line);
    if (result == NULL) {
        ast_dispose(&operand);
        parser_fail_internal(context, line);
        return false;
    }
    if (!parser_prec_stack_push(stack, PARSER_PREC_STACK_ITEM_NONTERMINAL, PARSER_PREC_COUNT, 0,
                                line, result)) {
        ast_dispose(&result);
        parser_fail_internal(context, line);
        return false;
    }
    return true;
}

static bool reduce_parenthesized_expression(parser_context_t *context, parser_prec_stack_t *stack) {
    parser_prec_stack_item_t *right_item = stack->top;
    parser_prec_stack_item_t *expression_item = right_item->next;
    parser_prec_stack_item_t *left_item = expression_item->next;
    if (left_item->type != PARSER_PREC_STACK_ITEM_TERMINAL ||
        left_item->index != PARSER_PREC_LPAREN ||
        expression_item->type != PARSER_PREC_STACK_ITEM_NONTERMINAL ||
        right_item->type != PARSER_PREC_STACK_ITEM_TERMINAL ||
        right_item->index != PARSER_PREC_RPAREN) {
        return false;
    }

    ast_node_t *expression = expression_item->node;
    int line = left_item->source_code_line;
    expression_item->node = NULL;
    parser_prec_stack_pop(stack);
    parser_prec_stack_pop(stack);
    parser_prec_stack_pop(stack);
    parser_prec_stack_pop(stack);

    if (!parser_prec_stack_push(stack, PARSER_PREC_STACK_ITEM_NONTERMINAL, PARSER_PREC_COUNT, 0,
                                line, expression)) {
        ast_dispose(&expression);
        parser_fail_internal(context, line);
        return false;
    }
    return true;
}

static bool reduce_binary_expression(parser_context_t *context, parser_prec_stack_t *stack) {
    parser_prec_stack_item_t *right_item = stack->top;
    parser_prec_stack_item_t *operator_item = right_item->next;
    parser_prec_stack_item_t *left_item = operator_item->next;
    if (left_item->type != PARSER_PREC_STACK_ITEM_NONTERMINAL ||
        operator_item->type != PARSER_PREC_STACK_ITEM_TERMINAL ||
        right_item->type != PARSER_PREC_STACK_ITEM_NONTERMINAL) {
        return false;
    }

    ast_node_t *left = left_item->node;
    ast_node_t *right = right_item->node;
    int line = operator_item->source_code_line;
    left_item->node = NULL;
    right_item->node = NULL;
    parser_prec_stack_pop(stack);
    parser_prec_stack_pop(stack);
    parser_prec_stack_pop(stack);
    parser_prec_stack_pop(stack);

    ast_node_t *operation = ast_expression_operation_create(
        parser_remap_operator(operator_item->token_type), left, right, line);
    if (operation == NULL) {
        ast_dispose(&left);
        ast_dispose(&right);
        parser_fail_internal(context, line);
        return false;
    }
    if (!parser_prec_stack_push(stack, PARSER_PREC_STACK_ITEM_NONTERMINAL, PARSER_PREC_COUNT, 0,
                                line, operation)) {
        ast_dispose(&operation);
        parser_fail_internal(context, line);
        return false;
    }
    return true;
}

bool parser_prec_table_reduce(parser_context_t *context, parser_prec_stack_t *stack) {
    int handle_length = 0;
    parser_prec_stack_item_t *current = stack->top;
    while (current != NULL && current->type != PARSER_PREC_STACK_ITEM_HANDLE_MARK) {
        handle_length++;
        current = current->next;
    }

    if (current == NULL) {
        parser_fail_syntax(context, context->current_token.line,
                           "Syntax error in expression: missing handle mark");
        return false;
    }

    if (handle_length == 1 && reduce_operand(context, stack)) {
        return true;
    }
    if (context->error_code == PARSE_ERR_INTERNAL) {
        return false;
    }
    if (handle_length == 2) {
        if (reduce_unit_literal(context, stack) || reduce_unary_expression(context, stack)) {
            return true;
        }
        if (context->error_code == PARSE_ERR_INTERNAL) {
            return false;
        }
    }
    if (handle_length == 3) {
        if (reduce_parenthesized_expression(context, stack) ||
            reduce_binary_expression(context, stack)) {
            return true;
        }
        if (context->error_code == PARSE_ERR_INTERNAL) {
            return false;
        }
    }

    parser_fail_syntax(context, context->current_token.line,
                       "Syntax error: no matching rule for expression reduction");
    return false;
}
