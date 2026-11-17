/*
-------------------------------------------------------
Name of project: Translator IFJ26
File: test_helpers.h

Description: Shared helpers for cmocka unit tests
             (in-memory scanner input, forked "death tests",
             per-test timeout against infinite loops).
-------------------------------------------------------
*/

#ifndef TEST_HELPERS_H
#define TEST_HELPERS_H

#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>

#include <cmocka.h>

#include "scanner.h"

// Per-test timeout (seconds). Use as setup function for every test group.
int test_setup_timeout(void **state);
int test_teardown_timeout(void **state);

// Opens a read-only FILE over a C string (empty string -> /dev/null).
FILE *open_source(const char *text);

// Scans the whole text and stores all token types in out (max max_tokens).
// Returns number of tokens including the terminating TOKEN_EOF.
int scan_all_types(const char *text, token_type_t *out, int max_tokens);

// Reads the first token of text.
token_t scan_first(const char *text);

// Runs get_token over the whole text in a forked child (stderr silenced).
// Returns exit status of the child (0 = scanned until EOF without exit()),
// or -1000-signal if the child died from a signal.
int scan_in_child(const char *text);

#endif
