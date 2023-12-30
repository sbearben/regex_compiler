CC = gcc
CCFLAGS = -std=gnu99 -Wall
INCLUDE = -I./
OUTDIR = bin

all: main

main: main.c regex.o parse.o dfa.o nfa.o list.o utils.o
	$(CC) $(CCFLAGS) $(INCLUDE) $^ -o ./$(OUTDIR)/$@

regex.o: regex.c
	$(CC) $(CCFLAGS) $(INCLUDE) regex.c -o $@ -c

parse.o: parse.c
	$(CC) $(CCFLAGS) $(INCLUDE) parse.c -o $@ -c

dfa.o: dfa.c
	$(CC) $(CCFLAGS) $(INCLUDE) dfa.c -o $@ -c

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
