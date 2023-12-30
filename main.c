#include <stdio.h>
#include <stdlib.h>

#include "regex.h"

// 1. Add runner method that confirms if a string is accepted by the dfa`
// 2. Change how nfa consumes the input stream of regex
// 3. Check for memory leaks?
// 4. Try DFA minimization?
// 5. Try NFA simulation?

int main(int argc, char** argv) {
   if (argc < 2) {
      printf("Usage: %s <regex>\n", argv[0]);
      return EXIT_FAILURE;
   }

   char* pattern = argv[1];
   regex_t* regex = new_regex(pattern);

   char input[256];
   while (scanf("Enter input: %s\n", input) != EOF) {
      printf("regex accepts: %s\n\n", regex_accepts(regex, input) ? "true" : "false");
   }

   regex_release(regex);

   return EXIT_SUCCESS;
}
