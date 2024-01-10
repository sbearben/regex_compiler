CC = gcc
CCFLAGS = -std=gnu99 -Wall
INCLUDE = -I./
OUTDIR = bin

TESTDIR = testing
T_LDFLAGS = -ldl
TF_CCFLAGS = $(CCFLAGS) -shared

# $^ = all dependencies
# $< = first dependency
# $@ = target
# -ldl = dynamic linking library
# -static = static linking flag

all: main tests

main: main.c sregex.o parse.o dfa.o nfa.o list.o utils.o
	$(CC) $(CCFLAGS) $(INCLUDE) $^ -o $(OUTDIR)/$@

sregex.o: sregex.c sregex.h
	$(CC) $(CCFLAGS) $(INCLUDE) $< -o $@ -c

parse.o: parse.c parse.h
	$(CC) $(CCFLAGS) $(INCLUDE) $< -o $@ -c

dfa.o: dfa.c dfa.h
	$(CC) $(CCFLAGS) $(INCLUDE) $< -o $@ -c

nfa.o: nfa.c nfa.h list.h
	$(CC) $(CCFLAGS) $< -o $@ -c

list.o: list.c list.h
	$(CC) $(CCFLAGS) $< -o $@ -c

utils.o: utils.c utils.h
	$(CC) $(CCFLAGS) $< -o $@ -c

## Testing

tests: test regex_test.so

test: test.o list.o
	$(CC) $(CCFLAGS) $(INCLUDE) $(T_LDFLAGS) $(TESTDIR)/test.o list.o -o $(OUTDIR)/$@

test.o: $(TESTDIR)/test.c $(TESTDIR)/test.h
	$(CC) $(CCFLAGS) $(INCLUDE) $< -o $(TESTDIR)/$@ -c

regex_test.so: $(TESTDIR)/regex_test.c sregex.o parse.o dfa.o nfa.o list.o utils.o
	$(CC) $(TF_CCFLAGS) $(INCLUDE) $(T_LDFLAGS) $^ -o ./$(TESTDIR)/$@

## Commands

.PHONY: clean format

clean:
	rm -f ./$(OUTDIR)/* *.o ./$(TESTDIR)/*.o ./$(TESTDIR)/*.so

format:
	clang-format -style=file -i *.c *.h
