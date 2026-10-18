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

token_t get_token(scanner_conf_t *conf) {

    // Creates variables for further use
    state_t state = STATE_START;
    token_t token;

    // State automaton
    while (1) {
        int ch = fgetc(conf->source_file);

        switch (state) {
        case STATE_START:
            if (ch == EOF) {
                token.type = TOKEN_EOF;
                return token;
            } else if (ch == '+') {
                token.type = TOKEN_PLUS;
                return token;
            }
            break;

        default:
            fprintf(stderr, "Lexical analysis Error, Invalid token type\n");
            exit(1); // Scanner error exit code
            break;
        }
    }
}
