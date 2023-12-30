#include <stdio.h>

#include "dfa.h"
#include "nfa.h"
#include "regex_nfa.h"

int main() {
   nfa_t* nfa = regex_to_nfa();
   log_nfa(nfa);
   printf("\n");

   dfa_t* dfa = dfa_from_nfa(nfa);
   log_dfa(dfa);

   free_nfa(nfa);
   free_dfa(dfa);

   return 0;
}
