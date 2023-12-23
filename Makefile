CC = gcc
CCFLAGS = -std=gnu99 -Wall

main: main.c parser.c dfa_machine.c
	$(CC) $(CCFLAGS) $^ -o $@ $(LDFLAGS)

.PHONY: clean format

clean:
	rm -f main *.o

format:
	clang-format -style=file -i *.c *.h
