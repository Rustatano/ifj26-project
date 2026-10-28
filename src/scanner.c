/*
-------------------------------------------------------
Name of project: Translator IFJ26
Authors: xzobals00 (Stepan Zobal)
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
    STATE_UNDERSCORE,
    // String
    STATE_FIR_QUOTE,
    STATE_SEC_QUOTE,
    STATE_THIRD_QUOTE,
    STATE_ONE_L_STR_DATA, // ONE LINE STRING DATA
    STATE_MUL_L_STR_DATA, // MULTIPLE LINE STRING DATA
    STATE_FIR_END_QUOTE,
    STATE_SEC_END_QUOTE,
    STATE_THIRD_END_QUOTE,
    STATE_ESC_SEQ_ONE_L, // ESCAPE SEQUENCE ONE LINE STRING
    STATE_ESC_SEQ_MUL_L, // ESCAPE SEQUENCE MULTI LINE STRING
    // Int
    STATE_WHOLE_NUM,
    STATE_WHOLE_NUM_UNDERSCORE,
    STATE_ZERO,
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

void string_init_check(dynamic_string_t *str) {
    if (!str_init(str)) {
        fprintf(stderr, "Lexical analysis Error 99: String allocation error\n");
        exit(99); // Internal compiler error
    }
}

void string_EOF_check(int ch) {
    if (ch == EOF) {
        fprintf(stderr, "Lexical analysis Error 1: Invalid string token, cannot include EOF \n");
        exit(1);
    }
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
    dynamic_string_t str; // For working with strings
    int char_buffer;      // Buffer for saving last char for conditions

    // State automaton
    while (1) {
        // Gets a character input
        int ch = get_char(conf);

        // Switches into a state
        switch (state) {
        case STATE_START:
            if (ch == EOF) {
                token.type = TOKEN_EOF;
                break;
            }
            // New line
            else if (ch == '\n') {
                token.type = TOKEN_EOL;
                break;
            }

            // Ignore white spaces
            else if (ch == ' ' || ch == '\r') {
                continue;
            }

            // Identifier
            else if (isalpha(ch)) {
                string_init_check(&str);
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
                break;
            } else if (ch == '-') {
                token.type = TOKEN_MINUS;
                break;
            } else if (ch == '*') {
                token.type = TOKEN_MUL;
                break;
            } else if (ch == '(') {
                token.type = TOKEN_LPAREN;
                break;
            } else if (ch == ')') {
                token.type = TOKEN_RPAREN;
                break;
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

            // Tabulator
            else if (ch == '\t') {
                fprintf(
                    stderr,
                    "Lexical analysis Error 1: Tabulator at the beginning of a non empty line\n");
                exit(1);
            }

            // String
            else if (ch == '"') {
                string_init_check(&str);
                state = STATE_FIR_QUOTE;
                continue;
            }

            // Whole number
            else if (isdigit(ch)) {
                token.type = TOKEN_INT;

                // Character to number conversion
                token.attr.int_val = ch - '0';

                if (ch == '0') {
                    state = STATE_ZERO;
                } else {
                    state = STATE_WHOLE_NUM;
                }
                continue;
            }
            break;

        case STATE_ASSIGN:
            // Checking for another token type
            if (ch == '=') {
                token.type = TOKEN_EQ;
                break;
            }
            // Next char is not "==" therefore it is an assign token
            // Storing unused char back for another read
            store_char(conf, ch);
            token.type = TOKEN_ASSIGN;
            break;

        case STATE_UNDERSCORE:
            // Identifier starting with underscore
            if (isalnum(ch) || ch == '_') {
                string_init_check(&str);
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
                // Set token type
                token.type = TOKEN_UNDERSCORE;
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

                // Set token type
                token.type = keyword_check(str.content);
                if (token.type == TOKEN_ID) {
                    token.attr.str_val = str.content;
                }
            }
            break;

        case STATE_LT:
            // Checking for another token type
            if (ch == '=') {
                token.type = TOKEN_LE;
            }
            // Storing excessive char back for another read
            else {
                store_char(conf, ch);
                token.type = TOKEN_LT;
            }
            break;

        case STATE_GT:
            // Checking for another token type
            if (ch == '=') {
                token.type = TOKEN_GE;
            }
            // Storing excessive char back for another read
            else {
                store_char(conf, ch);
                token.type = TOKEN_GT;
            }
            break;

        case STATE_NOT:
            // Checking for another token type
            if (ch == '=') {
                token.type = TOKEN_NEQ;
            }
            // Otherwise an error
            else {
                store_char(conf, ch);
                fprintf(stderr, "Lexical analysis Error 1: Invalid token '!' is not supported\n");
                exit(1);
            }
            break;

        case STATE_FIR_QUOTE:
            string_EOF_check(ch);
            // The input is [""]
            if (ch == '"') {
                state = STATE_SEC_QUOTE;
                continue;
            }
            // The input is ["A]
            else {
                str_add_char(&str, ch);
                state = STATE_ONE_L_STR_DATA;
                continue;
            }
            break;

        case STATE_ONE_L_STR_DATA:
            string_EOF_check(ch);
            // End of the string
            if (ch == '"') {
                token.type = TOKEN_STRING;
                token.attr.str_val = str.content;
                break;
            }
            // Next char is an special character
            else if (ch == '\n' || ch == '\r') {
                fprintf(stderr,
                        "Lexical analysis Error 1: Invalid One line string token, cannot "
                        "include: %c \n",
                        ch);
                exit(1);
            }
            // Next char is an escape sequence
            if (ch == '\\') {
                state = STATE_ESC_SEQ_ONE_L;
                continue;
            }
            // Next char is just another string data
            else {
                str_add_char(&str, ch);
                continue;
            }
            break;

        case STATE_ESC_SEQ_ONE_L:
            string_EOF_check(ch);
            // Next char is a valid escape sequence
            if (ch == '\\') {
                str_add_char(&str, '\\');
            } else if (ch == '"') {
                str_add_char(&str, '\"');
            } else if (ch == 'n') {
                str_add_char(&str, '\n');
            } else if (ch == 'r') {
                str_add_char(&str, '\r');
            } else if (ch == 't') {
                str_add_char(&str, '\t');
            }
            // Next char is not a valid sequence
            else {
                fprintf(stderr, "Lexical analysis Error 1: Not a valid escape sequence: '\\%c' \n",
                        ch);
                exit(1);
            }
            state = STATE_ONE_L_STR_DATA;
            continue;
            break; // Every case ends with an break that just defensive programing (compiler deletes
                   // it anyway)

        case STATE_SEC_QUOTE:
            // The whole input is a triple quote (beggining of a multi-line string)
            // lexeme state = ["""]
            if (ch == '"') {
                state = STATE_THIRD_QUOTE;
                continue;
            }
            // The end of an empty one-line string
            // [""]
            else {
                store_char(conf, ch);
                token.type = TOKEN_STRING;
                token.attr.str_val = str.content;
                // Token attribute is left empty (only with '\0') because there is no data
            }
            break;

        case STATE_THIRD_QUOTE:
            string_EOF_check(ch);
            // The input is a first end double quote
            // [""" "]
            if (ch == '"') {
                state = STATE_FIR_END_QUOTE;
                continue;
            }
            // There are data in the multi line string
            // [""" DATA ]
            else {
                state = STATE_MUL_L_STR_DATA;
                str_add_char(&str, ch);
                continue;
            }
            break;

        case STATE_MUL_L_STR_DATA:
            string_EOF_check(ch);
            // The input is a first ending double quote
            // [""" DATA "]
            if (ch == '"') {
                state = STATE_FIR_END_QUOTE;
                continue;
            }
            // The input is just data
            // [""" DATA NEW_DATA]
            else {
                str_add_char(&str, ch);
                continue;
            }
            break;

        case STATE_FIR_END_QUOTE:
            string_EOF_check(ch);
            // The input is a second ending double quote
            // [""" DATA ""]
            if (ch == '"') {
                state = STATE_SEC_END_QUOTE;
                continue;
            }
            // The previous double quote was just data, because it wasn't in sequence
            // with another two pairs of double quotes => next char is not a double quote
            // [""" DATA " DATA]
            else {
                state = STATE_MUL_L_STR_DATA;
                str_add_char(&str, '"');
                str_add_char(&str, ch);
                continue;
            }
            break;

        case STATE_SEC_END_QUOTE:
            string_EOF_check(ch);
            // The last double quote is found for the ending sequence of the multi line string
            // [""" DATA """]
            if (ch == '"') {
                token.type = TOKEN_STRING;
                token.attr.str_val = str.content;
                // Exit and return token
            }
            // The last two double quotes were just part of the string content not a finishing
            // sequence
            // [""" DATA "" DATA]
            else {
                state = STATE_MUL_L_STR_DATA;
                str_add_char(&str, '"');
                str_add_char(&str, '"');
                str_add_char(&str, ch);
                continue;
            }
            break;

            // Numbers

        case STATE_ZERO:
            // The input is a number
            if (isdigit(ch) || ch == '_') {
                fprintf(stderr,
                        "Lexical analysis Error 1: Invalid Number format, cannot accept a number "
                        "with a leading zero or a trailing underscore after zero: 0%c \n",
                        ch);
                exit(1);
            }
            // Return the unused non number char back for a read
            else {
                store_char(conf, ch);
                // Exit and return token
            }
            break;

        case STATE_WHOLE_NUM:
            if (isdigit(ch)) {
                // Decimal number shift plus the new number
                // (ch - '0') is a character to integer conversion
                token.attr.int_val = token.attr.int_val * 10 + (ch - '0');
                continue;
            }
            // Input is underscore, go to the underscore guard state
            else if (ch == '_') {
                state = STATE_WHOLE_NUM_UNDERSCORE;
                continue;
            }
            // The next character is a part of another token,
            // Puts it back for another read
            else {
                store_char(conf, ch);
                // Exit and return token
            }
            break;

        case STATE_WHOLE_NUM_UNDERSCORE:
            // The input is still an underscore, stay in this state
            if (ch == '_') {
                continue;
            }
            // The input is a whole number go back to the whole number state
            else if (isdigit(ch)) {
                state = STATE_WHOLE_NUM;
                // Decimal number shift plus the new number
                // (ch - '0') is a character to integer conversion
                token.attr.int_val = token.attr.int_val * 10 + (ch - '0');
            }
            // Number ends with an underscore
            else {
                fprintf(stderr,
                        "Lexical analysis Error 1: Number literal cannot end with an underscore "
                        "%d%c \n",
                        token.attr.int_val, ch);
                exit(1);
            }
            break;

        // If not in any valid state
        default:
            fprintf(stderr, "Lexical analysis Error 1: Invalid token type: %c \n", ch);
            exit(1);
            break;
        }
        return token;
    }
}
