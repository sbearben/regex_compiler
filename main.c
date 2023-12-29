#include <stdio.h>

#include "nfa.h"
#include "regex_nfa.h"

int main() {
   nfa_t* result = regex_to_nfa();
   log_nfa(result);
   free_nfa(result);

   return 0;
}
