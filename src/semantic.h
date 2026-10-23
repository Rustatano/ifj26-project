/*
-------------------------------------------------------
Name of project: Translator IFJ26
Authors: Zobal, Tajovský, Ondruška, Svoboda
Creation date: 18.10.2026
File: semantic.h

Description: Header file for semantic analyzer

-------------------------------------------------------
*/

#include <stdbool.h>

#include "ast.h"
#include "symbol_table.h"

bool semantic_init(symtable_stack_t *symtable_stack);

bool semantic_analysis(ast_node_t *ast, symtable_stack_t *symtable_stack);
