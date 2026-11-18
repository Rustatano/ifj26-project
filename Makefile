# documentation variables
DOC_DIR = doc
DOC_MAIN = dokumentace
DOC_BUILD_DIR = build

# archivation variables
ARCHIVE = ifj26_xsvoboj01_xzobals00_xtajovm00_xondrus00.zip

# source code variables
# detecting gcc-16 as a priority, then gcc-16.2 as second choice (located on Merlin server), fallback to default gcc
CC ?= $(shell \
    if command -v gcc-16 >/dev/null 2>&1; then echo gcc-16; \
    elif command -v gcc-16.2 >/dev/null 2>&1; then echo gcc-16.2; \
    else echo gcc; fi \
)
CFLAGS = -Wall -Wextra -Werror -pedantic -std=c11 -Isrc
SRC_DIR = src
SRC_FILES = $(filter-out $(SRC_DIR)/main.c, $(wildcard $(SRC_DIR)/*.c))
OUT = ifj26

# testing variables
TEST_OUT = test
TEST_DIR = tests

.PHONY: all build doc archive clean

# build project binary from source code
build:
	$(CC) $(CFLAGS) -o $(OUT) $(SRC_DIR)/*.c -lm

# all target is not on top, because of the need to run only build target when running just 'make' in terminal
all: build doc archive

# build documentation
doc:
	latexmk -pdf -interaction=nonstopmode -outdir=$(DOC_BUILD_DIR) -cd $(DOC_DIR)/$(DOC_MAIN).tex
	cp $(DOC_DIR)/$(DOC_BUILD_DIR)/$(DOC_MAIN).pdf .

# build archive
archive:
	rm -rf $(ARCHIVE)
	zip $(ARCHIVE) -r $(SRC_DIR)/ $(DOC_MAIN).pdf Makefile rozdeleni rozsireni

# check archive composition
check-archive:
	bash is_it_ok.sh $(ARCHIVE)

# check pre-commit and commit-msg hooks
check:
	pre-commit run --all-files

test:
	make -C tests/ all

# clean project from generated files
clean:
	rm -rf $(DOC_DIR)/$(DOC_BUILD_DIR) $(ARCHIVE) $(OUT) $(DOC_MAIN).pdf $(TEST_OUT)
