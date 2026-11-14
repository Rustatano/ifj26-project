#ifndef PARSE_FUNCTION_H
#define PARSE_FUNCTION_H

#include "parser_common.h"

ast_node_t *parse_fun_definition(parser_context_t *context, bool is_main);
ast_node_t *parse_return(parser_context_t *context);
#endif
