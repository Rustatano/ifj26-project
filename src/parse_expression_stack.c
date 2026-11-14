
#include "parse_expression_stack.h"

bool parser_prec_stack_push(parser_prec_stack_t *stack, parser_prec_stack_item_type_t item_type,
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

parser_prec_stack_item_t *parser_prec_stack_top_terminal(parser_prec_stack_t *stack) {
    parser_prec_stack_item_t *current = stack->top;
    while (current != NULL) {
        if (current->type == PARSER_PREC_STACK_ITEM_TERMINAL) {
            return current;
        }
        current = current->next;
    }
    return NULL;
}

bool parser_prec_stack_insert_handle_mark(parser_prec_stack_t *stack) {
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

void parser_prec_stack_pop(parser_prec_stack_t *stack) {
    if (!stack->top) {
        return;
    }
    parser_prec_stack_item_t *tmp = stack->top;
    stack->top = tmp->next;
    free(tmp);
}

void parser_prec_stack_dispose(parser_prec_stack_t *stack) {
    while (stack->top != NULL) {
        parser_prec_stack_item_t *tmp = stack->top;
        stack->top = tmp->next;
        if (tmp->node != NULL) {
            ast_dispose(&tmp->node);
        }
        free(tmp);
    }
}

bool parser_prec_stack_is_reduced(parser_prec_stack_t *stack) {
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
