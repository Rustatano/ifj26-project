/*
-------------------------------------------------------
Name of project: Translator IFJ26
File: test_helpers.c

Description: Implementation of shared helpers for unit tests.
-------------------------------------------------------
*/

#define _GNU_SOURCE
#include "test_helpers.h"
#include <fcntl.h>
#include <signal.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

#define TEST_TIMEOUT_SECONDS 5

static void timeout_handler(int sig) {
    (void)sig;
    // cmocka installs a handler for SIGABRT that marks the test as failed
    // and continues with the next one.
    raise(SIGABRT);
}

int test_setup_timeout(void **state) {
    (void)state;
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = timeout_handler;
    sa.sa_flags = SA_NODEFER; // handler must not stay blocked after longjmp
    sigemptyset(&sa.sa_mask);
    sigaction(SIGALRM, &sa, NULL);
    alarm(TEST_TIMEOUT_SECONDS);
    return 0;
}

int test_teardown_timeout(void **state) {
    (void)state;
    alarm(0);
    return 0;
}

FILE *open_source(const char *text) {
    size_t len = strlen(text);
    if (len == 0) {
        return fopen("/dev/null", "r");
    }
    return fmemopen((void *)text, len, "r");
}

int scan_all_types(const char *text, token_type_t *out, int max_tokens) {
    FILE *f = open_source(text);
    scanner_conf_t conf;
    scanner_init(&conf, f);
    int n = 0;
    while (n < max_tokens) {
        token_t t = get_token(&conf);
        out[n++] = t.type;
        if (t.type == TOKEN_EOF) {
            break;
        }
    }
    fclose(f);
    return n;
}

token_t scan_first(const char *text) {
    FILE *f = open_source(text);
    scanner_conf_t conf;
    scanner_init(&conf, f);
    token_t t = get_token(&conf);
    fclose(f);
    return t;
}

int scan_in_child(const char *text) {
    fflush(NULL);
    pid_t pid = fork();
    if (pid == 0) {
        int devnull = open("/dev/null", O_WRONLY);
        dup2(devnull, STDERR_FILENO);
        alarm(TEST_TIMEOUT_SECONDS);
        FILE *f = open_source(text);
        scanner_conf_t conf;
        scanner_init(&conf, f);
        for (int i = 0; i < 100000; i++) {
            token_t t = get_token(&conf);
            if (t.type == TOKEN_EOF) {
                break;
            }
        }
#ifdef SCANNER_HAS_RESULT
        // The scanner does not exit on errors, the error code is in its result
        _exit(conf.result.success ? 0 : conf.result.error_code);
#else
        // The scanner ends the process with exit(1) on a lexical error
        _exit(0);
#endif
    }
    int status = 0;
    waitpid(pid, &status, 0);
    if (WIFEXITED(status)) {
        return WEXITSTATUS(status);
    }
    if (WIFSIGNALED(status)) {
        return -1000 - WTERMSIG(status);
    }
    return -1;
}
