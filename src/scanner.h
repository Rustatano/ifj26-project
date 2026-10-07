/*
-------------------------------------------------------
Name of project: Translator IFJ26
Authors: Zobal, Tajovský, Ondruška, Svoboda
Creation date: 06.10.2026
File: scanner.h

Description: Header file for Lexical analyzator of the translator, which scans lexems for tokens

-------------------------------------------------------
*/

// Include Guard
#ifndef SCANNER_H
#define SCANNER_H

// Libraries:
#include <stdio.h>

// Enums:
typedef enum {
    TOKEN_ID,
    TOKEN_INT,
    TOKEN_DOUBLE,
    TOKEN_STRING,
    TOKEN_UNIT,

    // Operators
    TOKEN_PLUS,
    TOKEN_MINUS,
    TOKEN_MUL,
    TOKEN_DIV,

    // Relation
    TOKEN_ASSIGN,
    TOKEN_EQ,
    TOKEN_NEQ,
    TOKEN_GE,
    TOKEN_GT,
    TOKEN_LE,
    TOKEN_LT,

    // Keywords

    // Parentheses

    // Other

    TOKEN_EOF,

} token_type_t;

typedef union {
    double double_val;
    int int_val;
    char *str_val;
} token_value_t;

// Structs:
typedef struct {
    token_type_t type;
    token_value_t value;
} token_t;

// Functions:
token_t get_token();

#endif
