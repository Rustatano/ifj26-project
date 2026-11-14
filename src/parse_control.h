#ifndef PARSE_CONTROL_H
#define PARSE_CONTROL_H

#include "parser_common.h"

ast_node_t *parse_if(parser_context_t *context);
ast_node_t *parse_while(parser_context_t *context);
ast_node_t *parse_import(parser_context_t *context);
#endif
