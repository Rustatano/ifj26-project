/*
-------------------------------------------------------
Name of project: Translator IFJ26
File: test_scanner.c

Description: Unit tests (cmocka) of the lexical analyzer (scanner.c).
             Lexical errors in the scanner call exit(1), therefore
             they are tested in a forked child (scan_in_child).
-------------------------------------------------------
*/

#include "test_helpers.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

static void check_tokens(const char *src, const token_type_t *expected, int count) {
    token_type_t actual[256];
    int n = scan_all_types(src, actual, 256);
    assert_int_equal(n, count);
    for (int i = 0; i < count; i++) {
        if (actual[i] != expected[i]) {
            print_error("token #%d: expected %d, got %d (source: '%s')\n", i, expected[i],
                        actual[i], src);
        }
        assert_int_equal(actual[i], expected[i]);
    }
}

#define EXPECT_TOKENS(src, ...)                                                                    \
    do {                                                                                           \
        token_type_t exp_[] = {__VA_ARGS__};                                                       \
        check_tokens((src), exp_, (int)(sizeof(exp_) / sizeof(exp_[0])));                          \
    } while (0)

#define EXPECT_LEX_ERROR(src) assert_int_equal(scan_in_child(src), 1)
#define EXPECT_NO_LEX_ERROR(src) assert_int_equal(scan_in_child(src), 0)

// ---------------------------------------------------------------------------
// scanner_init
// ---------------------------------------------------------------------------

static void test_init_sets_initial_state(void **state) {
    (void)state;
    scanner_conf_t conf;
    FILE *f = open_source("");
    scanner_init(&conf, f);
    assert_ptr_equal(conf.source_file, f);
    assert_int_equal(conf.indent_top, 1);
    assert_int_equal(conf.indent_stack[0], 0);
    assert_int_equal(conf.unread_top, 0);
    assert_true(conf.line_start);
    assert_int_equal(conf.space_counter, 0);
    fclose(f);
}

// ---------------------------------------------------------------------------
// EOF, whitespace
// ---------------------------------------------------------------------------

static void test_empty_input_gives_eof(void **state) {
    (void)state;
    EXPECT_TOKENS("", TOKEN_EOF);
}

static void test_eof_is_returned_repeatedly(void **state) {
    (void)state;
    scanner_conf_t conf;
    FILE *f = open_source("");
    scanner_init(&conf, f);
    assert_int_equal(get_token(&conf).type, TOKEN_EOF);
    assert_int_equal(get_token(&conf).type, TOKEN_EOF);
    fclose(f);
}

static void test_spaces_between_tokens_are_ignored(void **state) {
    (void)state;
    EXPECT_TOKENS("a   +\t b", TOKEN_ID, TOKEN_PLUS, TOKEN_ID, TOKEN_EOF);
}

static void test_tab_inside_line_is_ok(void **state) {
    (void)state;
    EXPECT_NO_LEX_ERROR("a\t=\t1");
}

static void test_crlf_is_ignored(void **state) {
    (void)state;
    EXPECT_TOKENS("a\r\nb", TOKEN_ID, TOKEN_EOL, TOKEN_ID, TOKEN_EOF);
}

static void test_newline_gives_eol(void **state) {
    (void)state;
    EXPECT_TOKENS("a\nb", TOKEN_ID, TOKEN_EOL, TOKEN_ID, TOKEN_EOF);
}

// ---------------------------------------------------------------------------
// Keywords and identifiers
// ---------------------------------------------------------------------------

static void test_keywords(void **state) {
    (void)state;
    struct {
        const char *text;
        token_type_t type;
    } cases[] = {
        {"def", TOKEN_KW_DEF},       {"do", TOKEN_KW_DO},         {"else", TOKEN_KW_ELSE},
        {"if", TOKEN_KW_IF},         {"import", TOKEN_KW_IMPORT}, {"return", TOKEN_KW_RETURN},
        {"then", TOKEN_KW_THEN},     {"val", TOKEN_KW_VAL},       {"var", TOKEN_KW_VAR},
        {"while", TOKEN_KW_WHILE},   {"Int", TOKEN_KW_INT},       {"Double", TOKEN_KW_DOUBLE},
        {"String", TOKEN_KW_STRING}, {"Unit", TOKEN_KW_UNIT},
    };
    for (size_t i = 0; i < sizeof(cases) / sizeof(cases[0]); i++) {
        token_t t = scan_first(cases[i].text);
        if (t.type != cases[i].type) {
            print_error("keyword '%s' scanned as %d, expected %d\n", cases[i].text, t.type,
                        cases[i].type);
        }
        assert_int_equal(t.type, cases[i].type);
    }
}

static void test_keywords_are_case_sensitive(void **state) {
    (void)state;
    assert_int_equal(scan_first("Def").type, TOKEN_ID);
    assert_int_equal(scan_first("INT").type, TOKEN_ID);
    assert_int_equal(scan_first("int").type, TOKEN_ID);
    assert_int_equal(scan_first("string").type, TOKEN_ID);
}

static void test_keyword_prefix_is_identifier(void **state) {
    (void)state;
    token_t t = scan_first("define");
    assert_int_equal(t.type, TOKEN_ID);
    assert_string_equal(t.attr.str_val, "define");
    free(t.attr.str_val);

    t = scan_first("iff");
    assert_int_equal(t.type, TOKEN_ID);
    free(t.attr.str_val);
}

static void test_identifier_value(void **state) {
    (void)state;
    token_t t = scan_first("myVar_12");
    assert_int_equal(t.type, TOKEN_ID);
    assert_string_equal(t.attr.str_val, "myVar_12");
    free(t.attr.str_val);
}

static void test_identifier_starting_with_underscore(void **state) {
    (void)state;
    token_t t = scan_first("_abc");
    assert_int_equal(t.type, TOKEN_ID);
    assert_string_equal(t.attr.str_val, "_abc");
    free(t.attr.str_val);

    t = scan_first("__");
    assert_int_equal(t.type, TOKEN_ID);
    free(t.attr.str_val);

    t = scan_first("_1");
    assert_int_equal(t.type, TOKEN_ID);
    free(t.attr.str_val);
}

static void test_single_underscore_is_not_identifier(void **state) {
    (void)state;
    EXPECT_TOKENS("_ ", TOKEN_UNDERSCORE, TOKEN_EOF);
    EXPECT_TOKENS("_)", TOKEN_UNDERSCORE, TOKEN_RPAREN, TOKEN_EOF);
}

static void test_long_identifier_grows_buffer(void **state) {
    (void)state;
    char src[300];
    memset(src, 'a', sizeof(src) - 1);
    src[sizeof(src) - 1] = '\0';
    token_t t = scan_first(src);
    assert_int_equal(t.type, TOKEN_ID);
    assert_int_equal((int)strlen(t.attr.str_val), 299);
    free(t.attr.str_val);
}

// ---------------------------------------------------------------------------
// Operators and punctuation
// ---------------------------------------------------------------------------

static void test_arithmetic_operators(void **state) {
    (void)state;
    EXPECT_TOKENS("+ - * /", TOKEN_PLUS, TOKEN_MINUS, TOKEN_MUL, TOKEN_DIV, TOKEN_EOF);
}

static void test_relational_operators(void **state) {
    (void)state;
    EXPECT_TOKENS("= == != < <= > >=", TOKEN_ASSIGN, TOKEN_EQ, TOKEN_NEQ, TOKEN_LT, TOKEN_LE,
                  TOKEN_GT, TOKEN_GE, TOKEN_EOF);
}

static void test_operators_without_spaces(void **state) {
    (void)state;
    EXPECT_TOKENS("a<=b", TOKEN_ID, TOKEN_LE, TOKEN_ID, TOKEN_EOF);
    EXPECT_TOKENS("a==b", TOKEN_ID, TOKEN_EQ, TOKEN_ID, TOKEN_EOF);
    EXPECT_TOKENS("a=-1", TOKEN_ID, TOKEN_ASSIGN, TOKEN_MINUS, TOKEN_INT, TOKEN_EOF);
}

static void test_punctuation(void **state) {
    (void)state;
    EXPECT_TOKENS("( ) : ; , .", TOKEN_LPAREN, TOKEN_RPAREN, TOKEN_COLON, TOKEN_SEMICOLON,
                  TOKEN_COMMA, TOKEN_DOT, TOKEN_EOF);
}

static void test_unit_literal_is_two_tokens(void **state) {
    (void)state;
    EXPECT_TOKENS("()", TOKEN_LPAREN, TOKEN_RPAREN, TOKEN_EOF);
}

static void test_annotation_main(void **state) {
    (void)state;
    EXPECT_TOKENS("@main", TOKEN_ANNOTATION, TOKEN_EOF);
    EXPECT_TOKENS("@main def", TOKEN_ANNOTATION, TOKEN_KW_DEF, TOKEN_EOF);
}

static void test_import_prolog_tokens(void **state) {
    (void)state;
    EXPECT_TOKENS("import ifj26.*", TOKEN_KW_IMPORT, TOKEN_ID, TOKEN_DOT, TOKEN_MUL, TOKEN_EOF);
}

static void test_lone_exclamation_is_lexical_error(void **state) {
    (void)state;
    EXPECT_LEX_ERROR("!");
    EXPECT_LEX_ERROR("!a");
}

static void test_bad_annotation_is_lexical_error(void **state) {
    (void)state;
    EXPECT_LEX_ERROR("@");
    EXPECT_LEX_ERROR("@foo");
    EXPECT_LEX_ERROR("@mai");
    EXPECT_LEX_ERROR("@ main");
}

static void test_unknown_character_is_lexical_error(void **state) {
    (void)state;
    EXPECT_LEX_ERROR("#");
    EXPECT_LEX_ERROR("a $ b");
    EXPECT_LEX_ERROR("{");
    EXPECT_LEX_ERROR("a & b");
    EXPECT_LEX_ERROR("`x`");
}

// ---------------------------------------------------------------------------
// Int literals
// ---------------------------------------------------------------------------

static void test_int_literals(void **state) {
    (void)state;
    token_t t = scan_first("0");
    assert_int_equal(t.type, TOKEN_INT);
    assert_int_equal(t.attr.int_val, 0);

    t = scan_first("7");
    assert_int_equal(t.type, TOKEN_INT);
    assert_int_equal(t.attr.int_val, 7);

    t = scan_first("12345");
    assert_int_equal(t.type, TOKEN_INT);
    assert_int_equal(t.attr.int_val, 12345);
}

static void test_int_with_underscores(void **state) {
    (void)state;
    token_t t = scan_first("1_000_000");
    assert_int_equal(t.type, TOKEN_INT);
    assert_int_equal(t.attr.int_val, 1000000);
}

static void test_int_leading_zero_is_error(void **state) {
    (void)state;
    EXPECT_LEX_ERROR("007");
    EXPECT_LEX_ERROR("00");
    EXPECT_LEX_ERROR("0_1");
}

static void test_int_trailing_underscore_is_error(void **state) {
    (void)state;
    EXPECT_LEX_ERROR("1_");
    EXPECT_LEX_ERROR("1_ + 2");
}

static void test_negative_int_is_minus_then_int(void **state) {
    (void)state;
    EXPECT_TOKENS("-5", TOKEN_MINUS, TOKEN_INT, TOKEN_EOF);
}

static void test_int_followed_by_operator(void **state) {
    (void)state;
    EXPECT_TOKENS("1+2", TOKEN_INT, TOKEN_PLUS, TOKEN_INT, TOKEN_EOF);
    EXPECT_TOKENS("0)", TOKEN_INT, TOKEN_RPAREN, TOKEN_EOF);
}

// ---------------------------------------------------------------------------
// Double literals
// ---------------------------------------------------------------------------

static void check_double_token(const char *src, double expected) {
    token_t t = scan_first(src);
    if (t.type != TOKEN_DOUBLE) {
        print_error("'%s' scanned as token type %d, expected TOKEN_DOUBLE\n", src, t.type);
    }
    assert_int_equal(t.type, TOKEN_DOUBLE);
    double tolerance = fabs(expected) * 1e-9 + 1e-12;
    if (fabs(t.attr.double_val - expected) > tolerance) {
        print_error("'%s' scanned as %.17g, expected %.17g\n", src, t.attr.double_val, expected);
    }
    assert_true(fabs(t.attr.double_val - expected) <= tolerance);
}

static void test_double_basic(void **state) {
    (void)state;
    check_double_token("3.14", 3.14);
    check_double_token("0.5", 0.5);
    check_double_token("10.25", 10.25);
    check_double_token("0.001", 0.001);
}

static void test_double_with_exponent(void **state) {
    (void)state;
    check_double_token("1e3", 1000.0);
    check_double_token("1E3", 1000.0);
    check_double_token("1.5e2", 150.0);
    check_double_token("2.5E+2", 250.0);
    check_double_token("5e-1", 0.5);
    check_double_token("1.5e-2", 0.015);
    check_double_token("0e5", 0.0);
}

static void test_double_suffix(void **state) {
    (void)state;
    check_double_token("1.5d", 1.5);
    check_double_token("1.5D", 1.5);
    check_double_token("1e2d", 100.0);
}

static void test_double_without_integer_part(void **state) {
    (void)state;
    // [integer part] . {fraction}  -> integer part may be missing
    check_double_token(".5", 0.5);
}

static void test_double_with_underscores(void **state) {
    (void)state;
    check_double_token("1_0.2_5", 10.25);
    check_double_token("1.5e1_0", 1.5e10);
}

static void test_double_missing_fraction_digits_is_error(void **state) {
    (void)state;
    EXPECT_LEX_ERROR("1.");
    EXPECT_LEX_ERROR("1. ");
    EXPECT_LEX_ERROR("1.x");
}

static void test_double_bad_exponent_is_error(void **state) {
    (void)state;
    EXPECT_LEX_ERROR("1e");
    EXPECT_LEX_ERROR("1e+");
    EXPECT_LEX_ERROR("1e-");
    EXPECT_LEX_ERROR("1.5e");
    EXPECT_LEX_ERROR("1e+x");
}

static void test_double_trailing_underscore_is_error(void **state) {
    (void)state;
    EXPECT_LEX_ERROR("1.5_");
    EXPECT_LEX_ERROR("1.5e1_");
}

static void test_double_followed_by_operator(void **state) {
    (void)state;
    EXPECT_TOKENS("1.5+2.5", TOKEN_DOUBLE, TOKEN_PLUS, TOKEN_DOUBLE, TOKEN_EOF);
}

// ---------------------------------------------------------------------------
// String literals
// ---------------------------------------------------------------------------

static void check_string_token(const char *src, const char *expected) {
    token_t t = scan_first(src);
    assert_int_equal(t.type, TOKEN_STRING);
    assert_string_equal(t.attr.str_val, expected);
    free(t.attr.str_val);
}

static void test_simple_string(void **state) {
    (void)state;
    check_string_token("\"hello\"", "hello");
    check_string_token("\"a b c\"", "a b c");
    check_string_token("\"x\"", "x");
}

static void test_empty_string(void **state) {
    (void)state;
    check_string_token("\"\"", "");
    EXPECT_TOKENS("\"\" + \"a\"", TOKEN_STRING, TOKEN_PLUS, TOKEN_STRING, TOKEN_EOF);
}

static void test_string_escape_sequences(void **state) {
    (void)state;
    check_string_token("\"a\\nb\"", "a\nb");
    check_string_token("\"a\\tb\"", "a\tb");
    check_string_token("\"a\\rb\"", "a\rb");
    check_string_token("\"a\\\\b\"", "a\\b");
    check_string_token("\"say \\\"hi\\\"\"", "say \"hi\"");
}

static void test_string_only_escape(void **state) {
    (void)state;
    check_string_token("\"\\n\"", "\n");
}

static void test_string_invalid_escape_is_error(void **state) {
    (void)state;
    EXPECT_LEX_ERROR("\"a\\xb\"");
    EXPECT_LEX_ERROR("\"\\0\"");
    EXPECT_LEX_ERROR("\"\\a\"");
}

static void test_string_with_newline_is_error(void **state) {
    (void)state;
    EXPECT_LEX_ERROR("\"abc\ndef\"");
}

static void test_unterminated_string_is_error(void **state) {
    (void)state;
    EXPECT_LEX_ERROR("\"abc");
    EXPECT_LEX_ERROR("\"");
    EXPECT_LEX_ERROR("\"abc\\");
}

static void test_string_with_comment_chars_is_not_comment(void **state) {
    (void)state;
    check_string_token("\"// not comment\"", "// not comment");
    check_string_token("\"/* nor this */\"", "/* nor this */");
}

static void test_multiline_string(void **state) {
    (void)state;
    check_string_token("\"\"\"abc\"\"\"", "abc");
    check_string_token("\"\"\"A\nhoj\n\"\"\"", "A\nhoj\n");
}

static void test_multiline_string_does_not_evaluate_escapes(void **state) {
    (void)state;
    // spec: "hoj s\nvete" inside """ stays as backslash + n
    check_string_token("\"\"\"s\\nv\"\"\"", "s\\nv");
}

static void test_multiline_string_with_inner_quotes(void **state) {
    (void)state;
    check_string_token("\"\"\"He said \"hi\" ok\"\"\"", "He said \"hi\" ok");
    check_string_token("\"\"\"a\"\"b\"\"\"", "a\"\"b");
}

static void test_empty_multiline_string(void **state) {
    (void)state;
    check_string_token("\"\"\"\"\"\"", "");
}

static void test_unterminated_multiline_string_is_error(void **state) {
    (void)state;
    EXPECT_LEX_ERROR("\"\"\"abc");
    EXPECT_LEX_ERROR("\"\"\"abc\"\"");
}

// ---------------------------------------------------------------------------
// Comments
// ---------------------------------------------------------------------------

static void test_line_comment_gives_eol(void **state) {
    (void)state;
    EXPECT_TOKENS("a // comment\nb", TOKEN_ID, TOKEN_EOL, TOKEN_ID, TOKEN_EOF);
}

static void test_line_comment_at_eof(void **state) {
    (void)state;
    EXPECT_TOKENS("a // comment", TOKEN_ID, TOKEN_EOL, TOKEN_EOF);
}

static void test_division_is_not_comment(void **state) {
    (void)state;
    EXPECT_TOKENS("a / b", TOKEN_ID, TOKEN_DIV, TOKEN_ID, TOKEN_EOF);
}

static void test_inline_block_comment_acts_as_whitespace(void **state) {
    (void)state;
    EXPECT_TOKENS("a /* x */ b", TOKEN_ID, TOKEN_ID, TOKEN_EOF);
    EXPECT_TOKENS("a/**/b", TOKEN_ID, TOKEN_ID, TOKEN_EOF);
}

static void test_nested_block_comment(void **state) {
    (void)state;
    EXPECT_TOKENS("a /* x /* y */ z */ b", TOKEN_ID, TOKEN_ID, TOKEN_EOF);
}

static void test_block_comment_with_newline_acts_as_eol(void **state) {
    (void)state;
    EXPECT_TOKENS("a /* x\ny */\nb", TOKEN_ID, TOKEN_EOL, TOKEN_ID, TOKEN_EOF);
}

static void test_block_comment_with_newline_followed_by_token_is_error(void **state) {
    (void)state;
    EXPECT_LEX_ERROR("a /* x\ny */ b");
}

static void test_unterminated_block_comment_is_error(void **state) {
    (void)state;
    EXPECT_LEX_ERROR("/* abc");
    EXPECT_LEX_ERROR("/* /* abc */");
    EXPECT_LEX_ERROR("a /*");
}

// ---------------------------------------------------------------------------
// Indentation
// ---------------------------------------------------------------------------

static void test_indent_and_dedent(void **state) {
    (void)state;
    EXPECT_TOKENS("a\n  b\nc\n", TOKEN_ID, TOKEN_EOL, TOKEN_INDENT, TOKEN_ID, TOKEN_EOL,
                  TOKEN_DEDENT, TOKEN_ID, TOKEN_EOL, TOKEN_EOF);
}

static void test_nested_indent_and_double_dedent(void **state) {
    (void)state;
    EXPECT_TOKENS("a\n  b\n    c\nd\n", TOKEN_ID, TOKEN_EOL, TOKEN_INDENT, TOKEN_ID, TOKEN_EOL,
                  TOKEN_INDENT, TOKEN_ID, TOKEN_EOL, TOKEN_DEDENT, TOKEN_DEDENT, TOKEN_ID,
                  TOKEN_EOL, TOKEN_EOF);
}

static void test_same_indent_gives_no_indent_token(void **state) {
    (void)state;
    EXPECT_TOKENS("  a\n  b\n", TOKEN_INDENT, TOKEN_ID, TOKEN_EOL, TOKEN_ID, TOKEN_EOL, TOKEN_EOF);
}

static void test_tab_at_line_start_is_error(void **state) {
    (void)state;
    EXPECT_LEX_ERROR("a\n\tb");
    EXPECT_LEX_ERROR("a\n  \tb");
}

static void test_blank_lines_do_not_change_indentation(void **state) {
    (void)state;
    token_type_t tokens[64];
    int n = scan_all_types("a\n  b\n\n   \n  c\nd\n", tokens, 64);
    int indents = 0, dedents = 0;
    for (int i = 0; i < n; i++) {
        if (tokens[i] == TOKEN_INDENT) {
            indents++;
        }
        if (tokens[i] == TOKEN_DEDENT) {
            dedents++;
        }
    }
    // blank lines must not generate extra INDENT/DEDENT tokens
    assert_int_equal(indents, 1);
    assert_int_equal(dedents, 1);
}

static void test_dedent_to_unknown_level_is_error(void **state) {
    (void)state;
    // dedent to a column that was never an indentation level (spec: indentation error = 1)
    EXPECT_LEX_ERROR("a\n    b\n  c\n");
}

// ---------------------------------------------------------------------------
// Line numbers
// ---------------------------------------------------------------------------

static void test_token_line_numbers(void **state) {
    (void)state;
    scanner_conf_t conf;
    FILE *f = open_source("a\nb\nc");
    scanner_init(&conf, f);
    token_t t1 = get_token(&conf); // a
    get_token(&conf);              // EOL
    token_t t2 = get_token(&conf); // b
    get_token(&conf);              // EOL
    token_t t3 = get_token(&conf); // c
    assert_int_equal(t1.line, 1);
    assert_int_equal(t2.line, 2);
    assert_int_equal(t3.line, 3);
    fclose(f);
}

// ---------------------------------------------------------------------------
// Whole program
// ---------------------------------------------------------------------------

static void test_whole_small_program(void **state) {
    (void)state;
    EXPECT_TOKENS("import ifj26.*\n"
                  "@main def run(): Unit =\n"
                  "  val x = 1\n",
                  TOKEN_KW_IMPORT, TOKEN_ID, TOKEN_DOT, TOKEN_MUL, TOKEN_EOL, TOKEN_ANNOTATION,
                  TOKEN_KW_DEF, TOKEN_ID, TOKEN_LPAREN, TOKEN_RPAREN, TOKEN_COLON, TOKEN_KW_UNIT,
                  TOKEN_ASSIGN, TOKEN_EOL, TOKEN_INDENT, TOKEN_KW_VAL, TOKEN_ID, TOKEN_ASSIGN,
                  TOKEN_INT, TOKEN_EOL, TOKEN_EOF);
}

#define T(fn) cmocka_unit_test_setup_teardown(fn, test_setup_timeout, test_teardown_timeout)

int main(void) {
    const struct CMUnitTest tests[] = {
        T(test_init_sets_initial_state),
        T(test_empty_input_gives_eof),
        T(test_eof_is_returned_repeatedly),
        T(test_spaces_between_tokens_are_ignored),
        T(test_tab_inside_line_is_ok),
        T(test_crlf_is_ignored),
        T(test_newline_gives_eol),
        T(test_keywords),
        T(test_keywords_are_case_sensitive),
        T(test_keyword_prefix_is_identifier),
        T(test_identifier_value),
        T(test_identifier_starting_with_underscore),
        T(test_single_underscore_is_not_identifier),
        T(test_long_identifier_grows_buffer),
        T(test_arithmetic_operators),
        T(test_relational_operators),
        T(test_operators_without_spaces),
        T(test_punctuation),
        T(test_unit_literal_is_two_tokens),
        T(test_annotation_main),
        T(test_import_prolog_tokens),
        T(test_lone_exclamation_is_lexical_error),
        T(test_bad_annotation_is_lexical_error),
        T(test_unknown_character_is_lexical_error),
        T(test_int_literals),
        T(test_int_with_underscores),
        T(test_int_leading_zero_is_error),
        T(test_int_trailing_underscore_is_error),
        T(test_negative_int_is_minus_then_int),
        T(test_int_followed_by_operator),
        T(test_double_basic),
        T(test_double_with_exponent),
        T(test_double_suffix),
        T(test_double_without_integer_part),
        T(test_double_with_underscores),
        T(test_double_missing_fraction_digits_is_error),
        T(test_double_bad_exponent_is_error),
        T(test_double_trailing_underscore_is_error),
        T(test_double_followed_by_operator),
        T(test_simple_string),
        T(test_empty_string),
        T(test_string_escape_sequences),
        T(test_string_only_escape),
        T(test_string_invalid_escape_is_error),
        T(test_string_with_newline_is_error),
        T(test_unterminated_string_is_error),
        T(test_string_with_comment_chars_is_not_comment),
        T(test_multiline_string),
        T(test_multiline_string_does_not_evaluate_escapes),
        T(test_multiline_string_with_inner_quotes),
        T(test_empty_multiline_string),
        T(test_unterminated_multiline_string_is_error),
        T(test_line_comment_gives_eol),
        T(test_line_comment_at_eof),
        T(test_division_is_not_comment),
        T(test_inline_block_comment_acts_as_whitespace),
        T(test_nested_block_comment),
        T(test_block_comment_with_newline_acts_as_eol),
        T(test_block_comment_with_newline_followed_by_token_is_error),
        T(test_unterminated_block_comment_is_error),
        T(test_indent_and_dedent),
        T(test_nested_indent_and_double_dedent),
        T(test_same_indent_gives_no_indent_token),
        T(test_tab_at_line_start_is_error),
        T(test_blank_lines_do_not_change_indentation),
        T(test_dedent_to_unknown_level_is_error),
        T(test_token_line_numbers),
        T(test_whole_small_program),
    };
    return cmocka_run_group_tests_name("scanner", tests, NULL, NULL);
}
