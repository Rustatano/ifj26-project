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

typedef enum { STATE_START } state_t;

void scanner_init(scanner_conf_t *conf, FILE *file) {
    // Initializes all values for scanning

    conf->source_file = file;
    conf->indent_top = 0;
    conf->unread_top = 0;
}

// Reads next char, if there is chars on stack it will read them first before reading from input
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
        int ch = get_char(conf);

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

            // Errors
            else if (ch == '\t') {
                fprintf(
                    stderr,
                    "Lexical analysis Error, Tabulator at the beggining of then not empty line\n");
                exit(1); // Scanner error exit code
            }
            break;

        default:
            fprintf(stderr, "Lexical analysis Error, Invalid token type\n");
            exit(1); // Scanner error exit code
            break;
        }
    }
}
