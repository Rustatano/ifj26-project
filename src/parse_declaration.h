#ifndef PARSE_DECLARATION_H
#define PARSE_DECLARATION_H

#include "parser_common.h"

ast_node_t *parse_var_declaration(parser_context_t *context);
ast_node_t *parse_assignment(parser_context_t *context);
symtable_datatype_t parse_data_type(parser_context_t *context);

#endif
