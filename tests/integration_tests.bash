#!/bin/bash

# AI generated (Gemini)

# Configuration of paths and binaries
COMPILER="./ifj26"
INTERPRETER="./ic26int"
TEST_DIR="tests"

# Colors for clear output
GREEN='\033[0;32m'
RED='\033[0;31m'
YELLOW='\033[1;33m'
NC='\033[0m'

# Temporary files for test execution
TMP_CODE=$(mktemp)
TMP_OUT=$(mktemp)
TMP_ERR=$(mktemp)

# Cleanup temporary files on script exit
trap 'rm -f "$TMP_CODE" "$TMP_OUT" "$TMP_ERR"' EXIT

if [ ! -x "$COMPILER" ]; then
    echo -e "${RED}Error: Compiler '$COMPILER' does not exist or is not executable.${NC}"
    exit 1
fi

if [ ! -x "$INTERPRETER" ]; then
    echo -e "${RED}Error: Interpreter '$INTERPRETER' does not exist or is not executable.${NC}"
    exit 1
fi

PASSED=0
FAILED=0

for src_file in "$TEST_DIR"/*.ifj26; do
    [ -e "$src_file" ] || continue

    test_name=$(basename "$src_file" .ifj26)
    in_file="$TEST_DIR/$test_name.in"
    expected_out_file="$TEST_DIR/$test_name.out"
    expected_rc_file="$TEST_DIR/$test_name.rc"

    # Default expected compiler return code is 0
    expected_rc=0
    if [ -f "$expected_rc_file" ]; then
        expected_rc=$(tr -d '[:space:]' < "$expected_rc_file")
    fi

    # 1. Run compiler (stdin -> stdout, errors to stderr)
    "$COMPILER" < "$src_file" > "$TMP_CODE" 2> "$TMP_ERR"
    actual_rc=$?

    if [ "$actual_rc" -ne "$expected_rc" ]; then
        echo -e "[${RED}FAIL${NC}] $test_name: Compiler return code $actual_rc (expected $expected_rc)"
        [ -s "$TMP_ERR" ] && echo -e "       Stderr: $(cat "$TMP_ERR")"
        ((FAILED++))
        continue
    fi

    # If the compiler was expected to fail and ended with the correct code, the test passed
    if [ "$expected_rc" -ne 0 ]; then
        echo -e "[${GREEN}PASS${NC}] $test_name (caught expected error $expected_rc)"
        ((PASSED++))
        continue
    fi

    # 2. Run the interpreter on the generated intermediate code
    if [ -f "$in_file" ]; then
        "$INTERPRETER" "$TMP_CODE" < "$in_file" > "$TMP_OUT" 2> "$TMP_ERR"
    else
        "$INTERPRETER" "$TMP_CODE" < /dev/null > "$TMP_OUT" 2> "$TMP_ERR"
    fi
    int_rc=$?

    if [ "$int_rc" -ne 0 ]; then
        echo -e "[${RED}FAIL${NC}] $test_name: Interpreter failed with return code $int_rc"
        [ -s "$TMP_ERR" ] && echo -e "       Stderr: $(cat "$TMP_ERR")"
        ((FAILED++))
        continue
    fi

    # 3. Compare output using diff
    if [ -f "$expected_out_file" ]; then
        diff_output=$(diff -u "$expected_out_file" "$TMP_OUT")
    else
        diff_output=$(diff -u /dev/null "$TMP_OUT")
    fi

    if [ -z "$diff_output" ]; then
        echo -e "[${GREEN}PASS${NC}] $test_name"
        ((PASSED++))
    else
        echo -e "[${RED}FAIL${NC}] $test_name: Output mismatch (diff):"
        echo "$diff_output"
        ((FAILED++))
    fi
done

echo "----------------------------------------"
echo -e "Total: $((PASSED + FAILED)) | ${GREEN}Passed: $PASSED${NC} | ${RED}Failed: $FAILED${NC}"

[ "$FAILED" -eq 0 ]
