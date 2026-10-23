/*
-------------------------------------------------------
Name of project: Translator IFJ26
Authors: Zobal, Tajovský, Ondruška, Svoboda
Creation date: 06.10.2026
File: scanner.c

Description: Lexical analyzer, which scans characters
from input, groups them into lexems and returns tokens.

-------------------------------------------------------
*/

// Libraries
#include "scanner.h"
#include <ctype.h> // For isalpha(), ..
#include <stdbool.h>
#include <stdlib.h> // For malloc, realloc, ..
#include <string.h> // For strcmp, ..

// Macros
#define STRING_START_SIZE 64

// Enums
typedef enum {
    STATE_START,
    STATE_ASSIGN,
    STATE_LT,
    STATE_GT,
    STATE_NOT,
    STATE_ID,
    STATE_UNDERSCORE
} state_t;

// Structs
typedef struct {
    char *content; // Content of the string
    int length;    // Current string length (without '\0')
    int capacity;  // Allocated space
} dynamic_string_t;

// Functions

// Initializes string values
bool str_init(dynamic_string_t *str) {
    str->capacity = STRING_START_SIZE;
    str->length = 0;
    str->content = malloc(str->capacity);
    // Allocation not successful
    if (str->content == NULL) {
        return false;
    }
    str->content[0] = '\0';
    return true;
}

// Adds a character into a string
bool str_add_char(dynamic_string_t *str, char ch) {
    // If capacity is bigger than required new space
    if (str->length + 1 >= str->capacity) {
        // Increases string capacity
        int new_cap = str->capacity * 2;
        char *new_data = realloc(str->content, new_cap);
        // Couldn't increase capacity
        if (new_data == NULL) {
            return false;
        }
        // Update values
        str->content = new_data;
        str->capacity = new_cap;
    }
    str->content[str->length] = ch;
    str->length++;
    str->content[str->length] = '\0';
    return true;
}

// Checks if identifier is a valid keyword
token_type_t keyword_check(char *content) {
    // If the string matches a keyword, it frees the memory and returns a valid token type
    // Otherwise returns token type ID and keeps the memory
    if (strcmp(content, "def") == 0) {
        free(content);
        return TOKEN_KW_DEF;
    } else if (strcmp(content, "do") == 0) {
        free(content);
        return TOKEN_KW_DO;
    } else if (strcmp(content, "Double") == 0) {
        free(content);
        return TOKEN_KW_DOUBLE;
    } else if (strcmp(content, "else") == 0) {
        free(content);
        return TOKEN_KW_ELSE;
    } else if (strcmp(content, "if") == 0) {
        free(content);
        return TOKEN_KW_IF;
    } else if (strcmp(content, "import") == 0) {
        free(content);
        return TOKEN_KW_IMPORT;
    } else if (strcmp(content, "Int") == 0) {
        free(content);
        return TOKEN_KW_INT;
    } else if (strcmp(content, "return") == 0) {
        free(content);
        return TOKEN_KW_RETURN;
    } else if (strcmp(content, "String") == 0) {
        free(content);
        return TOKEN_KW_STRING;
    } else if (strcmp(content, "then") == 0) {
        free(content);
        return TOKEN_KW_THEN;
    } else if (strcmp(content, "Unit") == 0) {
        free(content);
        return TOKEN_KW_UNIT;
    } else if (strcmp(content, "val") == 0) {
        free(content);
        return TOKEN_KW_VAL;
    } else if (strcmp(content, "var") == 0) {
        free(content);
        return TOKEN_KW_VAR;
    } else if (strcmp(content, "while") == 0) {
        free(content);
        return TOKEN_KW_WHILE;
    } else {
        return TOKEN_ID;
    }
}

// Initializes all values for scanning
void scanner_init(scanner_conf_t *conf, FILE *file) {

    conf->source_file = file;
    conf->indent_top = 0;
    conf->unread_top = 0;
}

// Reads next char, if there are chars on stack it will read them first before reading from input
int get_char(scanner_conf_t *conf) {
    if (conf->unread_top > 0) {
        conf->unread_top--;
        return conf->unread_chars_stack[conf->unread_top];
    }
    return fgetc(conf->source_file);
}

// Stores chars to be processed for the next token
void store_char(scanner_conf_t *conf, int ch) {
    if (conf->unread_top < UNREAD_CHARS_SIZE) {
        conf->unread_chars_stack[conf->unread_top] = ch;
        conf->unread_top++;
    }
}

// Reads input file and processes first lexeme it finds
token_t get_token(scanner_conf_t *conf) {

    // Creates variables for further use
    state_t state = STATE_START;
    token_t token;
    dynamic_string_t str;
    int char_buffer;

    // State automaton
    while (1) {
        // Gets a character input
        int ch = get_char(conf);

        // Switches into a state
        switch (state) {
        case STATE_START:
            if (ch == EOF) {
                token.type = TOKEN_EOF;
                return token;
            }

            // Ignore white spaces
            else if (ch == ' ' || ch == '\r') {
                continue;
            }

            // Identifier
            else if (isalpha(ch)) {
                if (!str_init(&str)) {
                    fprintf(stderr, "Lexical analysis Error 99: String allocation error\n");
                    exit(99); // Internal compiler error
                }
                str_add_char(&str, ch);
                state = STATE_ID;
                continue;
            }
            // Underscore
            else if (ch == '_') {
                char_buffer = ch;
                state = STATE_UNDERSCORE;
                continue;
            }

            // Operators
            else if (ch == '+') {
                token.type = TOKEN_PLUS;
                return token;
            } else if (ch == '-') {
                token.type = TOKEN_MINUS;
                return token;
            } else if (ch == '*') {
                token.type = TOKEN_MUL;
                return token;
            } else if (ch == '(') {
                token.type = TOKEN_LPAREN;
                return token;
            } else if (ch == ')') {
                token.type = TOKEN_RPAREN;
                return token;
            }
            // Relations
            else if (ch == '=') {
                state = STATE_ASSIGN;
                continue;
            } else if (ch == '<') {
                state = STATE_LT;
                continue;
            } else if (ch == '>') {
                state = STATE_GT;
                continue;
            } else if (ch == '!') {
                state = STATE_NOT;
                continue;
            }

            // Errors
            else if (ch == '\t') {
                fprintf(
                    stderr,
                    "Lexical analysis Error 1: Tabulator at the beginning of a non empty line\n");
                exit(1); // Scanner error exit code
            }
            break;

        case STATE_ASSIGN:
            // Checking for another token type
            if (ch == '=') {
                token.type = TOKEN_EQ;
                return token;
            }
            // Next char is not "==" therefore it is an assign token
            // Storing unused char back for another read
            store_char(conf, ch);
            token.type = TOKEN_ASSIGN;
            return token;
            break;

        case STATE_UNDERSCORE:
            // Identifier starting with underscore
            if (isalnum(ch) || ch == '_') {
                if (!str_init(&str)) {
                    fprintf(stderr, "Lexical analysis Error 99: String allocation error\n");
                    exit(99); // Internal compiler error
                }
                // Saving both underscore and the char after in the string
                str_add_char(&str, char_buffer);
                str_add_char(&str, ch);
                // Moving to ID state
                state = STATE_ID;
                continue;
            }
            // Standalone underscore
            else {
                // Storing unused char back for another read
                store_char(conf, ch);
                // Return token
                token.type = TOKEN_UNDERSCORE;
                return token;
            }
            break;

        case STATE_ID:
            // The string keeps going
            if (isalnum(ch) || ch == '_') {
                str_add_char(&str, ch);
                continue;
            }
            // End of a string
            else {
                // Storing unused char back for another read
                store_char(conf, ch);

                // Return token
                token.type = keyword_check(str.content);
                if (token.type == TOKEN_ID) {
                    token.attr.str_val = str.content;
                }
                return token;
            }
            break;

        case STATE_LT:
            // Checking for another token type
            if (ch == '=') {
                token.type = TOKEN_LE;
                return token;
            }
            // Storing excessive char back for another read
            store_char(conf, ch);
            token.type = TOKEN_LT;
            return token;
            break;

        case STATE_GT:
            // Checking for another token type
            if (ch == '=') {
                token.type = TOKEN_GE;
                return token;
            }
            // Storing excessive char back for another read
            store_char(conf, ch);
            token.type = TOKEN_GT;
            return token;
            break;

        case STATE_NOT:
            // Checking for another token type
            if (ch == '=') {
                token.type = TOKEN_NEQ;
                return token;
            }
            // Otherwise an error
            store_char(conf, ch);
            fprintf(stderr, "Lexical analysis Error 1: Invalid token '!' is not supported\n");
            exit(1);
            break;

        // If not in any valid state
        default:
            fprintf(stderr, "Lexical analysis Error 1: Invalid token type\n");
            exit(1);
            break;
        }
    }
}
