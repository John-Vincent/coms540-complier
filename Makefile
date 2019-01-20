#---- VARIABLES
BIN = ./bin
DBIN = ./docs
SRC = ./src
DSRC = ./src/docs
OBJ = ./headers
CC = gcc
CFLAGS = -Werror -Wall -ggdb
TEXC = pdflatex
TEXFLAGS = -interaction=nonstopmode -output-directory $(DBIN)

C_BINARIES = $(addprefix $(BIN)/, $(addsuffix .o, temp ))
DOC_FILES = $(addprefix $(DBIN)/, $(addsuffix .pdf, developers))

#---- PHONY RULES
default: compiler documentation

compiler: $(BIN)/compiler
	@echo "making compiler"

docs: $(DOC_FILES)
	@echo "made documentation files"

spell: .tex
	@aspell -t -c .tex

makebin:
	@[  -d $(BIN) ] || echo "making bin folder"
	@[  -d $(BIN) ] || mkdir $(BIN)
	@[ -d $(DBIN) ] || echo "making docs folder"
	@[ -d $(DBIN) ] || mkdir $(DBIN)

clean:
	@if [ -d $(BIN) ]; then rm -r $(BIN); fi
	@if [ -d $(DBIN) ]; then rm -r $(DBIN); fi
	@echo "project directory is now clean"

.PHONY: default compiler docs makebin clean spell 

#---- COMPILATION RULES
$(DBIN)/%.pdf: $(DSRC)/%.tex | makebin
	@$(TEXC) $(TEXFLAGS) $< >> $(BIN)/latexgarbage.txt
	@rm $(DBIN)/*.log $(BIN)/latexgarbage.txt

$(BIN)/compiler: $(C_BINARIES) | makebin
	@$(CC) -o $@ $(CFLAGS) $(C_BINARIES)


$(BIN)/%.o: $(SRC)/%.c $(OBJ)/%.h
	@$(ECHO) compiling $<
	@$(CC) $(CFLAGS) -c $< -o $@


