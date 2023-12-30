#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "regex.h"

#define MAX_INPUT_SIZE 256

// 1. [DONE] Add runner method that confirms if a string is accepted by the dfa`
// 2. [DONE] Change how nfa consumes the input stream of regex
// 3. Add more regex functions (find all matches in input string, etc.);
// 4. Check for memory leaks?
// 5. Add tests
// 6. Try DFA minimization?
// 7. Try NFA simulation?

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
      printf("  ACCEPTS: %s\n", regex_accepts(regex, input) ? "true" : "false");
   }

   regex_release(regex);

   return EXIT_SUCCESS;
}
