CC = gcc
CCFLAGS = -std=gnu99 -Wall
INCLUDE = -I./
OUTDIR = bin

all: main

main: main.c regex_nfa.o dfa.o nfa.o list.o utils.o
	$(CC) $(CCFLAGS) $(INCLUDE) $^ -o ./$(OUTDIR)/$@

dfa.o: dfa.c
	$(CC) $(CCFLAGS) $(INCLUDE) dfa.c -o $@ -c

regex_nfa.o: regex_nfa.c regex_nfa.h
	$(CC) $(CCFLAGS) $(INCLUDE) regex_nfa.c -o $@ -c

nfa.o: nfa.c nfa.h list.h
	$(CC) $(CCFLAGS) nfa.c -o $@ -c

list.o: list.c list.h
	$(CC) $(CCFLAGS) list.c -o $@ -c

utils.o: utils.c utils.h
	$(CC) $(CCFLAGS) utils.c -o $@ -c

.PHONY: clean format

clean:
	rm -f ./$(OUTDIR)/* *.o

format:
	clang-format -style=file -i *.c *.h
