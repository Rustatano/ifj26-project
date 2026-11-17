# Makefile for the IFJ26 test suite (unit tests with cmocka + integration tests in bash)
#
#   make -C tests unit          build and run all unit tests
#   make -C tests integration   run integration tests (needs ../ifj26 and ../ic26int)
#   make -C tests all           both
#   make -C tests valgrind      unit tests under valgrind (memory errors)
#   make -C tests valgrind-leaks  the same including leaks
#
# Layout assumption: this file lives in <project>/tests/, sources in <project>/src/,
# the interpreter ic26int in <project>/.

SRC_DIR   := ../src
UNIT_DIR  := unit
BUILD_DIR := build

CC      ?= gcc
CFLAGS  := -std=c11 -g -O0 -Wall -Wextra -Werror -pedantic -Wno-unused-parameter -I$(SRC_DIR) -I$(UNIT_DIR)
LDLIBS  := -lcmocka -lm

# The scanner either ends the process on a lexical error (exit(1)) or reports it in conf->result
ifneq ($(shell grep -l scanner_result_t $(SRC_DIR)/scanner.h 2>/dev/null),)
CFLAGS += -DSCANNER_HAS_RESULT
endif

# Modules under test (everything except main.c)
SRC_FILES := $(filter-out $(SRC_DIR)/main.c, $(wildcard $(SRC_DIR)/*.c))
SRC_OBJS  := $(patsubst $(SRC_DIR)/%.c,$(BUILD_DIR)/src_%.o,$(SRC_FILES))

HELPER_OBJ := $(BUILD_DIR)/test_helpers.o

UNIT_SRCS  := $(wildcard $(UNIT_DIR)/test_*.c)
UNIT_NAMES := $(filter-out test_helpers,$(patsubst $(UNIT_DIR)/%.c,%,$(UNIT_SRCS)))
UNIT_BINS  := $(addprefix $(BUILD_DIR)/,$(UNIT_NAMES))

.PHONY: all unit integration valgrind valgrind-leaks clean
all: unit integration

$(BUILD_DIR):
	mkdir -p $@

$(BUILD_DIR)/src_%.o: $(SRC_DIR)/%.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(HELPER_OBJ): $(UNIT_DIR)/test_helpers.c $(UNIT_DIR)/test_helpers.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/test_%: $(UNIT_DIR)/test_%.c $(HELPER_OBJ) $(SRC_OBJS) $(UNIT_DIR)/test_helpers.h
	$(CC) $(CFLAGS) $< $(HELPER_OBJ) $(SRC_OBJS) -o $@ $(LDLIBS)

# Runs every unit test binary; continues after failure and reports overall status.
unit: $(UNIT_BINS)
	@fail=0; \
	for t in $(UNIT_BINS); do \
		echo "=== $$t ==="; \
		./$$t || fail=1; \
	done; \
	exit $$fail

integration:
	./run_integration_tests.sh

# Invalid reads/writes and uses of uninitialised values (leaks are not an error here)
valgrind: $(UNIT_BINS)
	@for t in $(UNIT_BINS); do \
		echo "=== valgrind $$t ==="; \
		valgrind --leak-check=no --error-exitcode=1 -q ./$$t || exit 1; \
	done

# Same including memory leaks
valgrind-leaks: $(UNIT_BINS)
	@for t in $(UNIT_BINS); do \
		echo "=== valgrind $$t ==="; \
		valgrind --leak-check=full --error-exitcode=1 -q ./$$t || exit 1; \
	done

clean:
	rm -rf $(BUILD_DIR)
