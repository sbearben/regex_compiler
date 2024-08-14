#include "sregex.h"

#include <assert.h>
#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "dfa.h"
#include "parse.h"
#include "utils.h"

struct regex {
      char* pattern;
      dfa_t* dfa;
};

static dfa_t* regex_parse(char*);

regex_t* new_regex(char* pattern) {
   regex_t* regex = xmalloc(sizeof(regex_t));
   regex->pattern = xmalloc(sizeof(char) * (strlen(pattern) + 1));
   strcpy(regex->pattern, pattern);
   regex->dfa = regex_parse(pattern);

   return regex;
}

bool regex_accepts(regex_t* regex, char* input) {
   return dfa_accepts(regex->dfa, input, strlen(input));
}

bool regex_test(regex_t* regex, char* input) {
   char* start = input;
   char* end = input + strlen(input);
   char* forward;

   // Calls to dfa_accepts() could be cached
   while (start < end) {
      forward = start + 1;
      while (forward <= end) {
         if (dfa_accepts(regex->dfa, start, forward - start)) {
            return true;
         }
         forward++;
      }
      start++;
   }
   return false;
}

void regex_release(regex_t* regex) {
   free(regex->pattern);
   free_dfa(regex->dfa);
   free(regex);
}

static dfa_t* regex_parse(char* pattern) {
   ast_node_t* ast = parse_regex(pattern);
   nfa_t* nfa = nfa_from_ast(ast);
   free_ast(ast);
   // log_nfa(nfa);

   dfa_t* dfa = dfa_from_nfa(nfa);
   free_nfa(nfa);

   // printf("\n");
   // log_dfa(dfa);

   return dfa;
}
