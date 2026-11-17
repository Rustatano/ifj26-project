#!/usr/bin/env bash
#
# Integration tests of the IFJ26 compiler (bash).
#
# Pipeline per test case:
#   compiler < case.ifj26 > case.code        (compare exit code with the expected one)
#   ic26int case.code < case.in > case.out   (compare exit code and stdout)
#
# Layout (relative to this script):
#   integration_test_codes/<category>/<name>.ifj26   source program
#                                     <name>.in         stdin for the interpreter (optional)
#                                     <name>.out        expected stdout of the interpreted program
#                                     <name>.rc         expected exit code of the COMPILER (default 0)
#                                     <name>.run_rc     expected exit code of the INTERPRETER (default 0)
#
# Expected compiler exit codes by category when no .rc file exists:
#   valid / runtime_error -> 0, lexical_error -> 1, syntax_error -> 2
#
# Usage:
#   ./run_integration_tests.sh [-v] [-k] [pattern]
#     -v        verbose: print stderr of the compiler and diffs of failed tests
#     -k        keep the generated .code files in ./integration_out/
#     pattern   run only tests whose path contains this substring
#
# Environment:
#   COMPILER  path to the compiler binary   (default: <project root>/ifj26)
#   INTERP    path to the IFJcode26 interpreter (default: <project root>/ic26int)
#   TIMEOUT   per-process timeout in seconds (default 10)

set -u

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"
CASES_DIR="$SCRIPT_DIR/integration_test_codes"
COMPILER="${COMPILER:-$ROOT_DIR/ifj26}"
INTERP="${INTERP:-$ROOT_DIR/ic26int}"
[ -x "$INTERP" ] || INTERP="$ROOT_DIR/ic26int-linux-x86_64"
TIMEOUT="${TIMEOUT:-10}"
OUT_DIR="$SCRIPT_DIR/integration_out"

VERBOSE=0
KEEP=0
PATTERN=""
while [ $# -gt 0 ]; do
    case "$1" in
        -v) VERBOSE=1 ;;
        -k) KEEP=1 ;;
        -h|--help) sed -n '2,32p' "$0"; exit 0 ;;
        *) PATTERN="$1" ;;
    esac
    shift
done

if [ -t 1 ]; then
    RED=$'\033[31m'; GREEN=$'\033[32m'; YELLOW=$'\033[33m'; RESET=$'\033[0m'
else
    RED=""; GREEN=""; YELLOW=""; RESET=""
fi

# ---- prerequisites ---------------------------------------------------------
# No compiler binary in the project root: build one from ../src into ./build/ifj26
if [ ! -x "$COMPILER" ]; then
    BUILT_COMPILER="$SCRIPT_DIR/build/ifj26"
    echo "Compiler '$COMPILER' not found, building $BUILT_COMPILER from $ROOT_DIR/src ..." >&2
    mkdir -p "$SCRIPT_DIR/build"
    if cc -std=c11 -g -O1 -Wall -Wextra -pedantic -I"$ROOT_DIR/src" "$ROOT_DIR"/src/*.c \
        -o "$BUILT_COMPILER" -lm; then
        COMPILER="$BUILT_COMPILER"
    fi
fi
if [ ! -x "$COMPILER" ]; then
    echo "${RED}ERROR${RESET}: compiler binary not found: $COMPILER" >&2
    echo "Build the project first or set COMPILER=/path/to/binary." >&2
    exit 2
fi
if [ ! -x "$INTERP" ]; then
    echo "${RED}ERROR${RESET}: interpreter not found or not executable: $INTERP" >&2
    echo "Put ic26int into the project root or set INTERP=/path/to/ic26int." >&2
    exit 2
fi

WORK_DIR="$(mktemp -d)"
trap 'rm -rf "$WORK_DIR"' EXIT
[ "$KEEP" -eq 1 ] && mkdir -p "$OUT_DIR"

# run <timeout> <stdin-file> <stdout-file> <stderr-file> cmd...   -> echoes exit code (124 = timeout)
run_cmd() {
    local t="$1" in="$2" out="$3" err="$4"
    shift 4
    timeout "$t" "$@" <"$in" >"$out" 2>"$err"
    echo $?
}

read_rc() { # file default
    if [ -f "$1" ]; then tr -d ' \n\r' <"$1"; else echo "$2"; fi
}

PASS=0
FAIL=0
FAILED_NAMES=()

report_fail() {
    FAIL=$((FAIL + 1))
    FAILED_NAMES+=("$1")
    printf '%sFAIL%s  %s\n' "$RED" "$RESET" "$1"
    [ -n "${2:-}" ] && printf '      %s\n' "$2"
}

report_pass() {
    PASS=$((PASS + 1))
    printf '%sPASS%s  %s\n' "$GREEN" "$RESET" "$1"
}

while IFS= read -r src; do
    rel="${src#"$CASES_DIR"/}"
    [ -n "$PATTERN" ] && [[ "$rel" != *"$PATTERN"* ]] && continue

    base="${src%.ifj26}"
    category="$(basename "$(dirname "$src")")"
    name="${rel%.ifj26}"

    case "$category" in
        lexical_error) default_rc=1 ;;
        syntax_error)  default_rc=2 ;;
        *)             default_rc=0 ;;
    esac
    expected_rc="$(read_rc "$base.rc" "$default_rc")"
    expected_run_rc="$(read_rc "$base.run_rc" 0)"

    code="$WORK_DIR/prog.code"
    cerr="$WORK_DIR/compiler.err"
    cout="$WORK_DIR/compiler.out"

    # ---- 1) compile -------------------------------------------------------
    rc="$(run_cmd "$TIMEOUT" "$src" "$code" "$cerr" "$COMPILER")"

    if [ "$rc" -eq 124 ]; then
        report_fail "$name" "compiler timed out (>${TIMEOUT}s)"
        continue
    fi
    if [ "$rc" -ge 128 ]; then
        report_fail "$name" "compiler crashed (signal $((rc - 128)))"
        [ "$VERBOSE" -eq 1 ] && sed 's/^/      | /' "$cerr" | head -5
        continue
    fi
    if [ "$rc" -ne "$expected_rc" ]; then
        report_fail "$name" "compiler exit code: expected $expected_rc, got $rc"
        [ "$VERBOSE" -eq 1 ] && sed 's/^/      | /' "$cerr" | head -5
        continue
    fi

    if [ "$expected_rc" -ne 0 ]; then
        # error expected: the compiler must not leave generated code on stdout
        if [ -s "$code" ]; then
            report_fail "$name" "compile error expected, but the compiler wrote code to stdout"
        else
            report_pass "$name"
        fi
        continue
    fi

    # ---- 2) generated code sanity --------------------------------------
    first_line="$(head -n 1 "$code")"
    if [ "$first_line" != ".IFJcode26" ]; then
        report_fail "$name" "first line of generated code must be '.IFJcode26', got '$first_line'"
        continue
    fi
    [ "$KEEP" -eq 1 ] && cp "$code" "$OUT_DIR/$(echo "$name" | tr '/' '_').code"

    # ---- 3) interpret -------------------------------------------------------
    input="$base.in"
    [ -f "$input" ] || input=/dev/null
    out="$WORK_DIR/prog.out"
    ierr="$WORK_DIR/interp.err"
    irc="$(run_cmd "$TIMEOUT" "$input" "$out" "$ierr" "$INTERP" "$code")"

    if [ "$irc" -eq 124 ]; then
        report_fail "$name" "interpreter timed out (>${TIMEOUT}s)"
        continue
    fi

    if [ "$irc" -ne "$expected_run_rc" ]; then
        report_fail "$name" "interpreter exit code: expected $expected_run_rc, got $irc"
        if [ "$VERBOSE" -eq 1 ]; then
            sed 's/^/      | /' "$ierr" | head -5
        fi
        continue
    fi

    # ---- 4) compare stdout ---------------------------------------------------
    expected_out="$base.out"
    if [ -f "$expected_out" ]; then
        if ! diff -q "$expected_out" "$out" >/dev/null; then
            report_fail "$name" "stdout differs from $(basename "$expected_out")"
            [ "$VERBOSE" -eq 1 ] && diff "$expected_out" "$out" | head -10 | sed 's/^/      | /'
            continue
        fi
    fi
    report_pass "$name"
done < <(find "$CASES_DIR" -name '*.ifj26' | sort)

TOTAL=$((PASS + FAIL))
echo
echo "----------------------------------------"
printf 'Total: %d   %sPassed: %d%s   %sFailed: %d%s\n' "$TOTAL" "$GREEN" "$PASS" "$RESET" \
    "$([ "$FAIL" -gt 0 ] && echo "$RED")" "$FAIL" "$RESET"
if [ "$FAIL" -gt 0 ]; then
    exit 1
fi
exit 0
