#include "regex.h"

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
   regex_t* regex = (regex_t*)xmalloc(sizeof(regex_t));
   regex->pattern = (char*)xmalloc(strlen(pattern) + 1);
   strcpy(regex->pattern, pattern);
   regex->dfa = regex_parse(pattern);

   return regex;
}

bool regex_accepts(regex_t* regex, char* input) {
   return dfa_accepts(regex->dfa, input, strlen(input));
}

void regex_release(regex_t* regex) {
   free(regex->pattern);
   free_dfa(regex->dfa);
   free(regex);
}

static dfa_t* regex_parse(char* pattern) {
   nfa_t* nfa = parse_regex_to_nfa();
   // log_nfa(nfa);
   // printf("\n");

   dfa_t* dfa = dfa_from_nfa(nfa);
   free_nfa(nfa);

   log_dfa(dfa);

   return dfa;
}
