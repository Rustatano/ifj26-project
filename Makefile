# documentation variables
DOC_DIR = doc
DOC_MAIN = dokumentace
DOC_BUILD_DIR = build

# archivation variables
ARCHIVE = ifj26_xsvoboj01_xzobals00_xtajovm00_xondrus00.zip

# source code variables
CC = gcc-16
CFLAGS = -Wall -Wextra -Werror -pedantic -std=c11 -Isrc
SRC_DIR = src
SRC_FILES = $(filter-out $(SRC_DIR)/main.c, $(wildcard $(SRC_DIR)/*.c))
OUT = ifj26

# testing variables
TEST_OUT = test
TEST_DIR = tests

.PHONY: all build doc archive clean

all: build doc archive

# build project binary from source code
build:
	$(CC) $(CFLAGS) -o $(OUT) $(SRC_DIR)/*.c -lm

# build documentation
doc:
	latexmk -pdf -outdir=$(DOC_BUILD_DIR) -cd $(DOC_DIR)/$(DOC_MAIN).tex
	cp $(DOC_DIR)/$(DOC_BUILD_DIR)/$(DOC_MAIN).pdf .

# build archive
archive:
	rm -rf $(ARCHIVE)
	zip $(ARCHIVE) -r $(SRC_DIR)/ $(TEST_DIR)/ $(DOC_MAIN).pdf Makefile rozdeleni

# check pre-commit and commit-msg hooks
check:
	pre-commit run --all-files

test-build:
	$(CC) $(CFLAGS) -o $(TEST_OUT) $(TEST_DIR)/*.c $(SRC_FILES) -lcmocka -lm

test: test-build
	./$(TEST_OUT)

# clean project from generated files
clean:
	rm -rf $(DOC_DIR)/$(DOC_BUILD_DIR) $(ARCHIVE) $(OUT) $(DOC_MAIN).pdf $(TEST_OUT)
