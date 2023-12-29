CC = gcc
CCFLAGS = -std=gnu99 -Wall
OUTDIR = bin

all: regex_nfa

regex_nfa: regex_nfa.c nfa.o list.o utils.o
	$(CC) $(CCFLAGS) $^ -o ./$(OUTDIR)/$@ 

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
