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
#include <ctype.h>  // For isalpha(), ..
#include <math.h>   // for pow()
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
    STATE_DOT,
    STATE_UNDERSCORE,
    STATE_LPAREN,
    STATE_AT, // @
    STATE_AT_M,
    STATE_AT_MA,
    STATE_AT_MAI,
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
    // Double
    STATE_DOUBLE_DOT,
    STATE_DOUBLE,
    STATE_DOUBLE_UNDERSCORE,
    STATE_DOUBLE_EXPO,
    STATE_DOUBLE_EXPO_SIGN,
    STATE_DOUBLE_EXPO_FIRST_DIGIT,
    STATE_DOUBLE_EXPO_UNDERSCORE,
    // Comments
    STATE_SLASH,
    STATE_COMMENT_LINE,
    STATE_COMMENT_BLOCK,
    STATE_COMMENT_BLOCK_STAR,
    STATE_COMMENT_BLOCK_SLASH,
    STATE_COMMENT_BLOCK_END_CHECK, // To check that after a block comment with '\n' has to be EOF
    // Indents, dedents
    STATE_INDENT_COUNT
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
        fprintf(stderr, "Lexical analysis Error 1: Invalid string, cannot include EOF \n");
        exit(1);
    }
}

void comment_EOF_check(int ch) {
    if (ch == EOF) {
        fprintf(stderr, "Lexical analysis Error 1: Invalid comment block, cannot end file without "
                        "ending the comment first \n");
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
    conf->indent_top = 1; // First unoccupied space
    conf->unread_top = 0; // First unoccupied space
    conf->line_start = true;

    conf->indent_stack[0] = 0; // Set zero space indent as the base indent
    conf->space_counter = 0;
    conf->line = 1;
    conf->paren_depth = 0;
    conf->newline_seen = false;
    conf->after_dedent = false;
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
    dynamic_string_t str;          // For working with strings
    int char_buffer = 0;           // Buffer for saving last char for conditions
    double decimal_scale = 1;      // For double numbers
    int exponent_value = 0;        // For double numbers
    bool exponent_sign_pos = true; // Exponent sign positive, for double numbers

    int comment_depth = 0;            // Comments
    bool comment_has_newline = false; // Comments

    token.attr.double_val = 0; // Set values for safety
    token.attr.int_val = 0;    // Set values for safety
    token.line = conf->line;

    // State automaton
    // Key structure elements
    // break; does break out of the switch and the while as well, always returns a token
    // continue; usually means switch to another state
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
                conf->line++;
                // Line break inside parentheses is just whitespace (spec 1.2)
                if (conf->paren_depth > 0) {
                    conf->newline_seen = true;
                    continue;
                }
                token.type = TOKEN_EOL;
                break;
            }

            // Ignore carriage return
            else if (ch == '\r') {
                continue;
            }

            // Indent, dedent detection
            else if (conf->line_start == true) {
                state = STATE_INDENT_COUNT;
                store_char(conf, ch);
                continue;
            }

            // Ignore spaces
            else if (ch == ' ' && conf->line_start == false) {
                continue;
            }

            // Tabulator in the middle of a line is okay
            else if (ch == '\t' && conf->line_start == false) {
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
            } else if (ch == '/') {
                state = STATE_SLASH;
                continue;
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

            // Other characters
            else if (ch == '@') {
                state = STATE_AT;
                continue;
            } else if (ch == ',') {
                token.type = TOKEN_COMMA;
                break;
            } else if (ch == ';') {
                token.type = TOKEN_SEMICOLON;
                break;
            } else if (ch == ':') {
                token.type = TOKEN_COLON;
                break;
            } else if (ch == '(') {
                conf->paren_depth++;
                token.type = TOKEN_LPAREN;
                break;
            } else if (ch == ')') {
                if (conf->paren_depth > 0) {
                    conf->paren_depth--;
                }
                token.type = TOKEN_RPAREN;
                break;
            } else if (ch == '.') {
                state = STATE_DOT;
                continue;
            }

            // String
            else if (ch == '"') {
                // Check malloc status
                string_init_check(&str);
                // Move to another state
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
            // Not a valid starting character for any token
            else {
                fprintf(stderr,
                        "Lexical analysis Error 1: Input doesn't match any valid token: %c\n", ch);
                exit(1);
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

        // Annotation @main
        case STATE_AT:
            // Next letter fits the format, move to another state
            if (ch == 'm') {
                state = STATE_AT_M;
                continue;
                // Doesn't match the correct format
            } else {
                store_char(conf, ch);
                fprintf(stderr, "Lexical analysis Error 1: Annotation in incorrect format\n");
                exit(1);
            }
            break;

        case STATE_AT_M:
            // Next letter fits the format, move to another state
            if (ch == 'a') {
                state = STATE_AT_MA;
                continue;
            }
            // Doesn't match the correct format
            else {
                store_char(conf, ch);
                fprintf(stderr, "Lexical analysis Error 1: Annotation in incorrect format\n");
                exit(1);
            }
            break;

        case STATE_AT_MA:
            // Next letter fits the format, move to another state
            if (ch == 'i') {
                state = STATE_AT_MAI;
                continue;
            }
            // Doesn't match the correct format
            else {
                store_char(conf, ch);
                fprintf(stderr, "Lexical analysis Error 1: Annotation in incorrect format\n");
                exit(1);
            }
            break;

        case STATE_AT_MAI:
            // Next letter fits the format, move to another state
            if (ch == 'n') {
                token.type = TOKEN_ANNOTATION;
            }
            // Doesn't match the correct format
            else {
                store_char(conf, ch);
                fprintf(stderr, "Lexical analysis Error 1: Annotation in incorrect format\n");
                exit(1);
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
            // The input is ["A] - the char is processed by the one line string state,
            // because it can be an escape sequence or an invalid line break
            else {
                store_char(conf, ch);
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
                if (ch == '\n') {
                    conf->line++;
                }
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
                if (ch == '\n') {
                    conf->line++;
                }
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
                if (ch == '\n') {
                    conf->line++;
                }
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
            // The next character is a dot
            else if (ch == '.') {
                token.attr.double_val = 0;
                state = STATE_DOUBLE_DOT;
                continue;
            }
            // The number has an exponent
            else if (ch == 'e' || ch == 'E') {
                state = STATE_DOUBLE_EXPO_SIGN;
                token.attr.double_val = 0;
                continue;
            }
            // Return the unused non-number char back for a read
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
                // The number has a decimal part
            } else if (ch == '.') {
                // Move the integer value into the token decimal value
                token.attr.double_val = token.attr.int_val;
                // And move to the Double subtree
                state = STATE_DOUBLE_DOT;
                continue;
            }
            // The number has an exponent
            else if (ch == 'e' || ch == 'E') {
                state = STATE_DOUBLE_EXPO_SIGN;
                // Move the Integer value into the double attribute
                token.attr.double_val = token.attr.int_val;
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
                continue;
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

        // There was NOT a number before the dot
        case STATE_DOT:
            // The dot is part of a double number
            if (isdigit(ch)) {
                token.type = TOKEN_DOUBLE;
                token.attr.double_val = token.attr.double_val + (ch - '0') / 10.0;
                decimal_scale = 100.0;
                state = STATE_DOUBLE;
                continue;
            }
            // It is just a dot operator
            else {
                store_char(conf, ch);
                token.type = TOKEN_DOT;
            }
            break;

        // There was a number before the dot
        case STATE_DOUBLE_DOT:
            // The number changes from integer to double
            if (isdigit(ch)) {
                token.type = TOKEN_DOUBLE;
                token.attr.double_val = token.attr.double_val + (ch - '0') / 10.0;
                decimal_scale = 100.0;
                state = STATE_DOUBLE;
                continue;
            }
            // Invalid double sequence
            else {
                store_char(conf, ch);
                fprintf(stderr,
                        "Lexical analysis Error 1: Invalid double number sequence double number "
                        "has to have a digit after a dot: "
                        "%d.%c \n",
                        (int)token.attr.double_val, ch);
                exit(1);
            }
            break;

        case STATE_DOUBLE:
            // The number continues
            if (isdigit(ch)) {
                // Adds the new digit to the double
                token.attr.double_val = token.attr.double_val + (ch - '0') / decimal_scale;
                decimal_scale = decimal_scale * 10;
                continue;
            }
            // The number ends with a suffix -d -D
            else if (ch == 'd' || ch == 'D') {
                token.type = TOKEN_DOUBLE;
                break;
            }
            //
            else if (ch == '_') {
                state = STATE_DOUBLE_UNDERSCORE;
                continue;
            } else if (ch == 'e' || ch == 'E') {
                state = STATE_DOUBLE_EXPO_SIGN;
                continue;
            }
            // The number has ended, return unused char back for another read
            else {
                store_char(conf, ch);
                token.type = TOKEN_DOUBLE;
            }
            break;

        case STATE_DOUBLE_UNDERSCORE:
            // The input is still an underscore, stay in this state
            if (ch == '_') {
                continue;
            }
            // The input is a number go back to the number state
            else if (isdigit(ch)) {
                state = STATE_DOUBLE;
                // Adds the new digit to the double
                // (ch - '0') is a character to integer conversion
                token.attr.double_val = token.attr.double_val + (ch - '0') / decimal_scale;
                decimal_scale = decimal_scale * 10;
                continue;
            }
            // Number ends with an underscore
            else {
                fprintf(stderr,
                        "Lexical analysis Error 1: Number literal cannot end with an underscore "
                        "%f%c \n",
                        token.attr.double_val, ch);
                exit(1);
            }
            break;

        case STATE_DOUBLE_EXPO_SIGN:
            // The exponent part is implicitly positive
            if (isdigit(ch)) {
                exponent_sign_pos = true;
                exponent_value = ch - '0';
                state = STATE_DOUBLE_EXPO; // Already have a first digit
                continue;
            }
            // The exponent is positive
            else if (ch == '+') {
                exponent_sign_pos = true;
                state = STATE_DOUBLE_EXPO_FIRST_DIGIT;
                continue;
            }
            // The exponent is negative
            else if (ch == '-') {
                exponent_sign_pos = false;
                state = STATE_DOUBLE_EXPO_FIRST_DIGIT;
                continue;
            }
            // Not a valid exponent sequence
            else {
                fprintf(stderr,
                        "Lexical analysis Error 1: Not a valid double exponent sequence: e%c \n",
                        ch);
                exit(1);
            }
            break;

        // The exponent has to contain at least one digit to be valid, so this state checks that
        case STATE_DOUBLE_EXPO_FIRST_DIGIT:
            if (isdigit(ch)) {
                // Shift the number and add the new digit
                // (ch - '0') is a char to int conversion
                exponent_value = exponent_value * 10 + (ch - '0');
                state = STATE_DOUBLE_EXPO;
                continue;
            }
            // The exponent line sequence is empty
            else {
                fprintf(stderr,
                        "Lexical analysis Error 1: Double number has exponent but it doesn't "
                        "contain any number in it \n");
                exit(1);
            }

        case STATE_DOUBLE_EXPO:
            // The exponent number continues, just add the digit to the whole value and continue
            // reading
            if (isdigit(ch)) {
                // Shift the number and add the new digit
                // (ch - '0') is a char to int conversion
                exponent_value = exponent_value * 10 + (ch - '0');
                continue;
            }
            // The exponent contains an underscore
            else if (ch == '_') {
                state = STATE_DOUBLE_EXPO_UNDERSCORE;
                continue;
            }
            // The number containg suffix -d -D and ends with it
            else if (ch == 'd' || ch == 'D') {
                // Multiply the double with the exponent
                if (exponent_sign_pos) {
                    token.attr.double_val = token.attr.double_val * pow(10.0, exponent_value);
                } else {
                    exponent_value = 0 - exponent_value;
                    token.attr.double_val = token.attr.double_val * pow(10.0, exponent_value);
                }
                // Return token
                token.type = TOKEN_DOUBLE;
                break;
            }
            // The number has ended, return unused char back for another read
            else {
                store_char(conf, ch);
                // Multiply the double with the exponent
                if (exponent_sign_pos) {
                    token.attr.double_val = token.attr.double_val * pow(10.0, exponent_value);
                } else {
                    exponent_value = 0 - exponent_value;
                    token.attr.double_val = token.attr.double_val * pow(10.0, exponent_value);
                }
                // Return token
                token.type = TOKEN_DOUBLE;
            }
            break;

        case STATE_DOUBLE_EXPO_UNDERSCORE:
            // The input is still an underscore, stay in this state
            if (ch == '_') {
                continue;
            }
            // The input is a number go back to the double exponent state
            else if (isdigit(ch)) {
                state = STATE_DOUBLE_EXPO;
                // Decimal number shift plus the new number
                // (ch - '0') is a character to integer conversion
                exponent_value = exponent_value * 10 + (ch - '0');
                continue;
            }
            // Number ends with an underscore
            else {
                fprintf(stderr,
                        "Lexical analysis Error 1: Number exponent cannot end with an underscore "
                        "%d%c \n",
                        exponent_value, ch);
                exit(1);
            }
            break;

        // Comments
        case STATE_SLASH:
            // The input is a line comment
            if (ch == '/') {
                state = STATE_COMMENT_LINE;
                continue;
            }
            // The input is an block comment
            else if (ch == '*') {
                state = STATE_COMMENT_BLOCK;
                continue;
            }
            // The slash was just an operator division
            else {
                token.type = TOKEN_DIV;
                // Return token
            }
            break;

        case STATE_COMMENT_LINE:
            // The line comment has ended
            if (ch == '\n' || ch == EOF) {
                if (ch == '\n') {
                    conf->line++;
                    // Inside parentheses the line break is only whitespace
                    if (conf->paren_depth > 0) {
                        conf->newline_seen = true;
                        state = STATE_START;
                        continue;
                    }
                }
                token.type = TOKEN_EOL;
            }
            // The line comment continues
            else {
                continue;
            }
            break;

        case STATE_COMMENT_BLOCK:
            // Check for end of file in the block comment
            comment_EOF_check(ch);

            // The comment contains a new line
            if (ch == '\n') {
                conf->line++;
                comment_has_newline = true;
                continue;
            }
            // The input is a ending comment block character sequence
            else if (ch == '*') {
                state = STATE_COMMENT_BLOCK_STAR;
                continue;
            }
            // The input is a starting comment block character sequence
            else if (ch == '/') {
                state = STATE_COMMENT_BLOCK_SLASH;
                continue;
            }
            // The input is just a comment data
            else {
                continue;
            }
            break;

        case STATE_COMMENT_BLOCK_STAR:
            // Check for end of file in the block comment
            comment_EOF_check(ch);

            // The input is a finished comment block ending sequence
            if (ch == '/') {
                // The block comment isn't inside another block comment
                if (comment_depth == 0) {
                    // Multiline block comment inside parentheses: ignore EOL, mark newline seen,
                    // and continue
                    if (comment_has_newline && conf->paren_depth > 0) {
                        conf->newline_seen = true;
                        comment_depth = 0;
                        comment_has_newline = false;
                        state = STATE_START;
                        continue;
                    }
                    // Multiline block comment outside parentheses: must be followed by EOL or EOF
                    else if (comment_has_newline && conf->paren_depth == 0) {
                        state = STATE_COMMENT_BLOCK_END_CHECK; // Check for EOL after the comment
                        continue;
                    }
                    // Start reading from the start state, the comment has ended
                    else {
                        // Set the variables clean for next lexeme
                        comment_depth = 0;
                        comment_has_newline = false;
                        state = STATE_START;
                        continue;
                    }
                }
                // The block comment is inside another block comment
                else if (comment_depth > 0) {
                    comment_depth--;
                    state = STATE_COMMENT_BLOCK;
                    continue;
                }
                // Else state just to be safe
                else {
                    fprintf(stderr,
                            "Lexical analysis Error 1: The comment depth is somehow negative");
                    exit(1);
                }
            }
            // Check for new line
            else if (ch == '\n') {
                conf->line++;
                comment_has_newline = true;
                state = STATE_COMMENT_BLOCK;
                continue;
            }
            // The input didn't finish block comment ending sequence it was just comment data
            else {
                store_char(conf, ch);
                state = STATE_COMMENT_BLOCK;
                continue;
            }
            break;

        case STATE_COMMENT_BLOCK_SLASH:
            // Check for end of file in the block comment
            comment_EOF_check(ch);

            // New comment block has started
            if (ch == '*') {
                comment_depth++;
                state = STATE_COMMENT_BLOCK;
                continue;
            }
            // Check for new line
            else if (ch == '\n') {
                conf->line++;
                comment_has_newline = true;
                state = STATE_COMMENT_BLOCK;
                continue;
            }
            // The comment block starting sequence was not finished it was just comment data
            else {
                store_char(conf, ch);
                state = STATE_COMMENT_BLOCK;
                continue;
            }
            break;

        case STATE_COMMENT_BLOCK_END_CHECK:
            // Ignore space characters
            if (ch == ' ' || ch == '\r' || ch == '\t') {
                continue;
            }
            // EOF found the block with newlines is valid
            else if (ch == '\n' || ch == EOF) {
                // If EOF was found store it for another read as s separate token after EOL token
                if (ch == EOF) {
                    store_char(conf, ch);
                } else {
                    conf->line++;
                }
                token.type = TOKEN_EOL;
                break;
            }
            // After the block comment with new line is not EOL
            else {
                fprintf(stderr, "Lexical analysis Error 1: Invalid character after a block comment "
                                "containing a newline \n");
                exit(1);
            }
            break;

        case STATE_INDENT_COUNT:
            // Counting spaces
            if (ch == ' ') {
                conf->space_counter++;
                continue;
            }
            // New line resets the counting by returning EOL token
            else if (ch == '\n') {
                // Reset the counter
                conf->space_counter = 0;
                conf->line++;

                token.type = TOKEN_EOL;
                break;
            }
            // Found tabulator while reading the spaces
            else if (ch == '\t') {
                fprintf(
                    stderr,
                    "Lexical analysis Error 1: Tabulator at the beginning of a non empty line\n");
                exit(1);
            }
            // First non empty character found
            else {
                // Makes scanner return first all the indent tokens before finally returning the EOF
                // token
                if (ch == EOF) {
                    conf->space_counter = 0;
                }

                // A line containing only a line comment is ignored (it does not change indentation)
                if (ch == '/') {
                    int next_ch = get_char(conf);
                    if (next_ch == '/') {
                        int comment_ch;
                        while ((comment_ch = get_char(conf)) != '\n' && comment_ch != EOF) {
                        }
                        conf->space_counter = 0;
                        if (comment_ch == '\n') {
                            conf->line++;
                        } else {
                            store_char(conf, EOF);
                        }
                        token.type = TOKEN_EOL;
                        break;
                    }
                    // The "/" is not a comment beggining, return the char after it back for a read
                    store_char(conf, next_ch);
                }

                // Store unused char for another read
                store_char(conf, ch);

                // Checks if we have found more spaces than last indentation
                if (conf->space_counter > conf->indent_stack[conf->indent_top - 1]) {
                    // Dedent to a column that is not an indentation level of any open block
                    if (conf->after_dedent) {
                        fprintf(stderr,
                                "Lexical analysis Error 1: Inconsistent indentation "
                                "(dedent to unknown level) on line %d\n",
                                conf->line);
                        exit(1);
                    }
                    if (conf->indent_top >= INDENT_STACK_SIZE) {
                        fprintf(stderr, "Lexical analysis Error 1: Indentation is nested too "
                                        "deeply\n");
                        exit(1);
                    }
                    // Sets new space count as the stack top
                    conf->indent_stack[conf->indent_top] = conf->space_counter;
                    // Increments the top
                    conf->indent_top++;

                    // Reset the space counter
                    conf->space_counter = 0;

                    // Returns indent token
                    token.type = TOKEN_INDENT;
                    break;
                }
                // There are less spaces than before
                else if (conf->space_counter < conf->indent_stack[conf->indent_top - 1]) {

                    // Decrements the top
                    conf->indent_top--;
                    conf->after_dedent = true;

                    // Returns dedent token
                    token.type = TOKEN_DEDENT;
                    break;
                }
                // The line is at the same indentation as the last
                else {

                    // Set a clean table for reading further
                    conf->space_counter = 0;
                    conf->after_dedent = false;
                    conf->line_start = false; // Sets indentation as done for this line
                    state = STATE_START;
                    continue;
                }
            }
            break;

        // If not in any valid state
        default:
            fprintf(stderr, "Lexical analysis Error 1: Invalid token type: %c \n", ch);
            exit(1);
            break;
        } // End of switch case

        // Set line start variable for indentation
        if (token.type == TOKEN_EOL || token.type == TOKEN_DEDENT) {
            conf->line_start = true;
        } else {
            conf->line_start = false;
        }

        // Update token line after skipping lines and NOT returning EOL with comments and
        // parentheses. EOL itself belongs to the ended line
        if (token.type == TOKEN_EOL) {
            token.line = conf->line - 1;
        } else {
            token.line = conf->line;
        }

        // Remember whether a line break inside parentheses preceded this token
        token.newline_before = conf->newline_seen;
        conf->newline_seen = false;
        // Finally return token to parser
        return token;
    }
}
