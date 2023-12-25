CC = gcc
CCFLAGS = -std=gnu99 -Wall
OUTDIR = bin

all: regex_nfa

regex_nfa: regex_nfa.c #dfa_machine.c
	$(CC) $(CCFLAGS) $^ -o ./$(OUTDIR)/$@ $(LDFLAGS)

.PHONY: clean format

clean:
	rm -f ./$(OUTDIR)/*

format:
	clang-format -style=file -i *.c *.h
