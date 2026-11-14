/*
-------------------------------------------------------
Name of project: Translator IFJ26
Authors: Ondruška
Creation date: 10.10.2026
File: parser.c

Description: Parser implementation.

-------------------------------------------------------
*/
#include "parser.h"
#include "parse_control.h"
#include "parse_function.h"
#include "parser_common.h"
#include <stdbool.h>

// Helper function for copying the error state (code, line, message) from parser context to parse
// result
static void set_parse_result_error(parse_result_t *result, const parser_context_t *context) {
    result->error_code = context->error_code;
    result->error_line = context->error_line;
    result->error_msg = context->error_message;
}

// Main entry point for parsing the entire IFJ26 source program into an AST
// {EOLS}
// import ifj26.*
// {EOLS}
// [@main]
// def {id}([{param_id}: {DataType}, ...]) [: {DataType}] = ...
parse_result_t parse(ast_node_t **ast_root, scanner_conf_t *scanner_conf) {
    parse_result_t result = {
        .success = false, .error_code = PARSE_SUCCESS, .error_line = 0, .error_msg = NULL};

    if (ast_root == NULL || scanner_conf == NULL) {
        result.error_code = PARSE_ERR_INTERNAL;
        result.error_msg = "Invalid parameters passed to parse()";
        return result;
    }
    *ast_root = NULL;

    parser_context_t context = {.scanner_conf = scanner_conf,
                                .error_line = 0,
                                .error_message = NULL,
                                .error_code = PARSE_SUCCESS};

    load_token(&context);

    while (check_token(&context, TOKEN_EOL)) {
        load_token(&context);
    }

    ast_node_t *root = ast_block_create(context.current_token.line);
    if (root == NULL) {
        parser_fail_internal(&context, context.current_token.line);
        set_parse_result_error(&result, &context);
        return result;
    }

    ast_node_t *prolog = parse_import(&context);
    if (prolog == NULL) {
        ast_dispose(&root);
        result.error_line = context.error_line;
        result.error_msg = context.error_message;
        result.error_code = context.error_code;
        return result;
    }
    ast_block_append(root, prolog);

    bool main_seen = false;
    while (!check_token(&context, TOKEN_EOF)) {
        while (check_token(&context, TOKEN_EOL)) {
            load_token(&context);
        }
        if (check_token(&context, TOKEN_EOF)) {
            break;
        }

        bool is_main = false;
        if (check_token(&context, TOKEN_ANNOTATION)) {
            is_main = true;
            if (main_seen) {
                ast_dispose(&root);
                parser_fail_syntax(&context, context.current_token.line,
                                   "Duplicate @main annotation");
                set_parse_result_error(&result, &context);
                return result;
            }
            main_seen = true;
            load_token(&context);
            while (check_token(&context, TOKEN_EOL)) {
                load_token(&context);
            }
        }

        ast_node_t *stmt = NULL;
        if (check_token(&context, TOKEN_KW_DEF)) {
            stmt = parse_fun_definition(&context, is_main);
        } else {
            parser_fail_syntax(&context, context.current_token.line,
                               is_main ? "Expected function definition after @main"
                                       : "Only function definitions are allowed at top level");
        }
        if (stmt == NULL) {
            ast_dispose(&root);
            *ast_root = NULL;

            result.success = false;
            result.error_code = context.error_code;
            result.error_line = context.error_line;
            result.error_msg = context.error_message;
            return result;
        }

        ast_block_append(root, stmt);
    }

    if (main_seen == false) {
        ast_dispose(&root);
        parser_fail_syntax(&context, context.current_token.line,
                           "Program must contain a function annotated with @main");
        set_parse_result_error(&result, &context);
        return result;
    }

    *ast_root = root;
    result.success = true;
    result.error_code = PARSE_SUCCESS;
    return result;
}
