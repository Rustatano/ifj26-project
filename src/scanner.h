/*
-------------------------------------------------------
Name of project: Translator IFJ26
Authors: xzobals00 (Stepan Zobal)
Creation date: 06.10.2026
File: scanner.h

Description: Header file for lexical analyzer, which scans characters
from input, groups them into lexems and returns tokens.

-------------------------------------------------------
*/

// Include guard
#ifndef SCANNER_H
#define SCANNER_H

// Libraries
#include <stdio.h>

// Macros
#define UNREAD_CHARS_SIZE 10
#define INDENT_STACK_SIZE 100

// Enums
typedef enum {
    // Datatypes
    TOKEN_ID,
    TOKEN_INT,
    TOKEN_DOUBLE,
    TOKEN_STRING,
    // Operators
    TOKEN_PLUS,
    TOKEN_MINUS,
    TOKEN_MUL,
    TOKEN_DIV,

    // Relational operators
    TOKEN_ASSIGN,
    TOKEN_EQ,
    TOKEN_NEQ,
    TOKEN_GE,
    TOKEN_GT,
    TOKEN_LE,
    TOKEN_LT,

    // Keywords
    TOKEN_KW_DEF,
    TOKEN_KW_DO,
    TOKEN_KW_THEN,
    TOKEN_KW_IF,
    TOKEN_KW_ELSE,
    TOKEN_KW_IMPORT,
    TOKEN_KW_RETURN,
    TOKEN_KW_VAL,
    TOKEN_KW_VAR,
    TOKEN_KW_WHILE,

    // Keywords - data types
    TOKEN_KW_INT,
    TOKEN_KW_DOUBLE,
    TOKEN_KW_STRING,
    TOKEN_KW_UNIT,

    // Parentheses
    TOKEN_LPAREN,
    TOKEN_RPAREN,

    // Special characters
    TOKEN_DOT,
    TOKEN_COLON,
    TOKEN_SEMICOLON,
    TOKEN_COMMA,
    TOKEN_ANNOTATION,
    TOKEN_UNDERSCORE,

    // Indentation
    TOKEN_INDENT,
    TOKEN_DEDENT,

    // Other
    TOKEN_EOL,
    TOKEN_EOF,

} token_type_t;

// Unions
typedef union {
    double double_val;
    int int_val;
    char *str_val;
} token_attr_t;

// Structs
typedef struct {
    token_type_t type;
    token_attr_t attr;
    int line;
} token_t;

typedef struct {
    FILE *source_file;
    int unread_chars_stack[UNREAD_CHARS_SIZE];
    int unread_top; // First unoccupied space
    int indent_stack[INDENT_STACK_SIZE];
    int indent_top; // First unoccupied space

} scanner_conf_t;

// Functions

// Initializes scanner structures needed for reading file and making tokens
void scanner_init(scanner_conf_t *conf, FILE *file);

// Reads characters from file pointer and gathers them into a token
token_t get_token(scanner_conf_t *conf);

#endif
