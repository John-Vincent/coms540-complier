#---- VARIABLES

#directories
BIN = ./bin
DBIN = ./docs
SRC = ./src
DSRC = ./src/docs
DEFINITIONS = ./includes
LIBDIRS = core,lexer

#commands
CC = gcc
TEXC = pdflatex
LEX = flex
YACC = bison

#options
TEXFLAGS = -interaction=nonstopmode -output-directory $(DBIN)
CFLAGS = -Werror -Wall -ggdb
CLIBS = -lfl

#targets
C_CORE = $(addprefix core/, main hashmap utils)
LEXER =  $(addprefix lexer/, c_lang.yy lexer)
PARSER = $(addprefix parser/, c_parser.tab parser)
TYPE = $(addprefix type_checker/, symbol_table)
CODE_GEN = $(addprefix code_gen/, intermediate_generator)
C_BINARIES = $(addprefix $(BIN)/, $(addsuffix .o, $(PARSER) $(C_CORE) $(LEXER) $(TYPE) $(CODE_GEN) ))
VM_BINARY = $(addprefix $(BIN)/, code_gen/stackvm.o)
DOC_FILES = $(addprefix $(DBIN)/, $(addsuffix .pdf, developers))
SYMBOL_TEST_FILES = $(addprefix src/, $(addprefix core/, utils.c hashmap.c) type_checker/symbol_table.c)

#---- PHONY RULES
default: compile docs

compile: $(BIN)/compile
	@echo "made compile"

vm: $(BIN)/vm
	@echo "made vm"

docs: $(DOC_FILES)
	@echo "made documentation files"

symbol_test: $(SYMBOL_TEST_FILES) | $(BIN)/.
	@echo "making symbol table test"
	@$(CC) $(CFLAGS) -DTEST_SYMBOL_TABLE $(SYMBOL_TEST_FILES) -o bin/symbol_test

spell: .tex
	@aspell -t -c .tex

clean:
	@if [ -d $(BIN) ]; then rm -r $(BIN); fi
	@if [ -d $(DBIN) ]; then rm -r $(DBIN); fi
	@echo "project directory is now clean"

.PHONY: default compile docs clean spell 

#---- COMPILATION RULES

.PRECIOUS: $(BIN)%/. $(BIN)/. $(DBIN)/. $(BIN)/%.yy.c $(BIN)/%.tab.c

#rules for making an bin directories needed
$(BIN)/.:
	@mkdir -p $@

$(BIN)%/.:
	@mkdir -p $@

$(DBIN)/.:
	@mkdir -p $@

#allow for creating bin directories on the fly
.SECONDEXPANSION:

#make to run latex twice to get the page numbers generated.
$(DBIN)/%.pdf: $(DSRC)/%.tex | $$(@D)/.
	@$(TEXC) $(TEXFLAGS) $< >> $(BIN)/latexgarbage.txt
	@$(TEXC) $(TEXFLAGS) $< >> $(BIN)/latexgarbage.txt
	@rm $(DBIN)/*.log $(BIN)/latexgarbage.txt

$(BIN)/compile: $(C_BINARIES) | $$(@D)/.
	@echo "linking objects"
	@$(CC) $(CFLAGS) $(C_BINARIES) $(CLIBS) -o $@

$(BIN)/vm: $(VM_BINARY) | $$(@D)/.
	@$(CC) $(CFLAGS) $(VM_BINARY) -o $@

#standard c object rule
$(BIN)/%.o: $(SRC)/%.c | $$(@D)/. 
	@echo "compiling $<"
	@$(CC) $(CFLAGS) -c $< -o $@

#c object from generated c file
$(BIN)/%.o: $(BIN)/%.c | $$(@D)/.
	@echo "compiling $<"
	@$(CC) $(CFLAGS) -Wno-unused-function -c $<  -o $@

$(BIN)/%.yy.c: $(SRC)/%.l | $$(@D)/.
	@$(LEX) -o $@ $<

$(BIN)/%.tab.c: $(SRC)/%.y | $$(@D)/.
	@$(YACC) --defines=$(BIN)/parser/bison.h --output=$@ $< 
