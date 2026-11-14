#ifndef PARSER_COMMON_H
#define PARSER_COMMON_H

#include "parser.h"

typedef struct parser_context {
    token_t current_token;
    int error_line;
    const char *error_message;
    parse_error_code error_code;
    scanner_conf_t *scanner_conf;
    token_t buffered_token;
    bool has_buffered_token;
    symtable_datatype_t current_return_type;
} parser_context_t;

void load_token(parser_context_t *context);
void unread_token(parser_context_t *context, token_t token);
bool parse_argument_list(parser_context_t *context, ast_node_t **params, int *params_count);
bool check_token(parser_context_t *context, token_type_t token_type);
bool expect_token(parser_context_t *context, token_type_t token_type, const char *error_message);
bool is_statement_terminator(token_type_t type);
void dispose_parameters(symtable_fun_param_t **parameters);
void *parser_fail_internal(parser_context_t *context, int error_line);
void *parser_fail_syntax(parser_context_t *context, int error_line, const char *error_message);
bool consume_token(parser_context_t *context, token_type_t expected, const char *error_message);
void skip_eols(parser_context_t *context);

#endif
