CC = gcc
CCFLAGS = -std=gnu99 -Wall
INCLUDE = -I./
OUTDIR = bin

# Test lib
TESTLIB = lib/testing
TLDFLAGS = -ldl

# Test files
TF_DIR = tests
TF_CCFLAGS = $(CCFLAGS) -shared
TF_INCLUDE = $(INCLUDE) -I./$(TESTLIB)

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

tests: test regex_test.so parse_test.so

test: test.o list.o
	$(CC) $(CCFLAGS) $(INCLUDE) $(TLDFLAGS) $(TESTLIB)/test.o list.o -o $(OUTDIR)/$@

test.o: $(TESTLIB)/test.c $(TESTLIB)/test.h
	$(CC) $(CCFLAGS) $(INCLUDE) $< -o $(TESTLIB)/$@ -c

regex_test.so: $(TF_DIR)/regex_test.c sregex.o parse.o dfa.o nfa.o list.o utils.o
	$(CC) $(TF_CCFLAGS) $(TF_INCLUDE) $^ -o ./$(TF_DIR)/$@

parse_test.so: $(TF_DIR)/parse_test.c parse.o nfa.o list.o utils.o
	$(CC) $(TF_CCFLAGS) $(TF_INCLUDE) $^ -o ./$(TF_DIR)/$@

## Commands

.PHONY: clean format

clean:
	rm -f ./$(OUTDIR)/* *.o ./$(TESTLIB)/*.o ./$(TF_DIR)/*.so

format:
	clang-format -style=file -i *.c *.h
