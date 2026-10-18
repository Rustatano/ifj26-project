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
    TOKEN_KEYWORD_DEF,
    TOKEN_KEYWORD_DO,
    TOKEN_KEYWORD_THEN,
    TOKEN_KEYWORD_IF,
    TOKEN_KEYWORD_ELSE,
    TOKEN_KEYWORD_IMPORT,
    TOKEN_KEYWORD_RETURN,
    TOKEN_KEYWORD_VAL,
    TOKEN_KEYWORD_VAR,
    TOKEN_KEYWORD_WHILE,

    // Keywords - data types
    TOKEN_KEYWORD_INT,
    TOKEN_KEYWORD_DOUBLE,
    TOKEN_KEYWORD_STRING,
    TOKEN_KEYWORD_UNIT,

    // Parentheses
    TOKEN_LPAREN,
    TOKEN_RPAREN,

    // Special characters
    TOKEN_COLON,
    TOKEN_SEMICOLON,
    TOKEN_COMMA,
    TOKEN_ANNOTATION,

    // Spacing
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
