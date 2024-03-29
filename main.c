#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "sregex.h"

#define MAX_INPUT_SIZE 256

// 1. [DONE] Add runner method that confirms if a string is accepted by the dfa`
// 2. [DONE] Change how nfa consumes the input stream of regex
// 3. [DONE] Rename regex.c/regex.h to something that doens't conflict with the POSIX regex.h
// 4. Add more regex functions (find all matches in input string, etc.);
// 5. [DONE - fixed leaks] Check for memory leaks?
// 6. [DONE] Add tests
// 7. Use this to generate a lexical-analyzer generator?
// 8. Try DFA minimization?
// 9. Try NFA simulation?
// 10. Construct the DFA directly by algorithm 3.36 in dragon book (p. 204)

void read_line(char* buffer, int size) {
   int i = 0;
   char c;
   while ((c = getchar()) != '\n' && i < size - 1) {
      buffer[i++] = c;
   }
   if (i == 0) {
      buffer[i++] = '\n';
   }
   buffer[i] = '\0';
}

int main(int argc, char** argv) {
   if (argc < 2) {
      printf("Usage: %s <regex>\n", argv[0]);
      return EXIT_FAILURE;
   }

   char* pattern = argv[1];
   regex_t* regex = new_regex(pattern);

   char input[MAX_INPUT_SIZE];
   while (input[0] != '\n') {
      printf("\n");
      printf("Pattern: %s\n", pattern);
      printf("  Input: ");
      read_line(input, MAX_INPUT_SIZE);
      printf("  Result: %s\n", regex_accepts(regex, input) ? "ACCEPTED" : "NOT ACCEPTED");
   }

   regex_release(regex);

   return EXIT_SUCCESS;
}
