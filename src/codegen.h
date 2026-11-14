/*
-------------------------------------------------------
Name of project: Translator IFJ26
Authors: xtajovm00 (Matej Tajovsky)
Creation date: 02.11.2026
File: codegen.c

Description: Header file for code generation

-------------------------------------------------------
*/

#ifndef CODEGEN_H
#define CODEGEN_H

#include "ast.h"
#include "symbol_table.h"
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

bool codegen(ast_node_t *ast_root, const char *output_file);

#endif
