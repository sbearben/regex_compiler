CC = gcc
CCFLAGS = -std=gnu99 -Wall
OUTDIR = bin

all: regex_nfa

regex_nfa: regex_nfa.c nfa.o list.o utils.o
	$(CC) $(CCFLAGS) $^ -o ./$(OUTDIR)/$@ 

nfa.o: nfa.c list.o
	$(CC) $(CCFLAGS) $^ -o $@ -c

list.o: list.c
	$(CC) $(CCFLAGS) $^ -o $@ -c

utils.o: utils.c
	$(CC) $(CCFLAGS) $^ -o $@ -c

.PHONY: clean format

clean:
	rm -f ./$(OUTDIR)/* *.o

format:
	clang-format -style=file -i *.c *.h
