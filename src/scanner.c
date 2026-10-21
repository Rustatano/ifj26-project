/*
-------------------------------------------------------
Name of project: Translator IFJ26
Authors: Zobal, Tajovský, Ondruška, Svoboda
Creation date: 06.10.2026
File: scanner.c

Description: Lexical analyzator of the translator, which scans lexems for tokens

-------------------------------------------------------
*/

// Libraries
#include "scanner.h"
#include <stdlib.h>

typedef enum { STATE_START, STATE_ASSIGN, STATE_LT, STATE_GT, STATE_NOT } state_t;

void scanner_init(scanner_conf_t *conf, FILE *file) {
    // Initializes all values for scanning

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

// Stores chars to be proccesed for the next token
void store_char(scanner_conf_t *conf, int ch) {
    if (conf->unread_top < UNREAD_CHARS_SIZE) {
        conf->unread_chars_stack[conf->unread_top] = ch;
        conf->unread_top++;
    }
}

token_t get_token(scanner_conf_t *conf) {

    // Creates variables for further use
    state_t state = STATE_START;
    token_t token;

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
            // Storing excessive char back for another read
            store_char(conf, ch);
            token.type = TOKEN_ASSIGN;
            return token;
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

        default:
            fprintf(stderr, "Lexical analysis Error 1: Invalid token type\n");
            exit(1); // Scanner error exit code
            break;
        }
    }
}
