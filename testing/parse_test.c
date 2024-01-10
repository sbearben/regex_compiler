#include "parse.h"

#include "test_file.h"

TEST_CASE(creates_correct_number_states) {
   nfa_t* nfa;

   nfa = parse_regex_to_nfa("a");
   assert_int_equal(nfa_num_states(nfa), 2);
   free_nfa(nfa);

   nfa = parse_regex_to_nfa("ab");
   assert_int_equal(nfa_num_states(nfa), 4);
   free_nfa(nfa);

   nfa = parse_regex_to_nfa("a*");
   assert_int_equal(nfa_num_states(nfa), 4);
   free_nfa(nfa);

   nfa = parse_regex_to_nfa("a|b");
   assert_int_equal(nfa_num_states(nfa), 6);
   free_nfa(nfa);
}

void on_register_tests(void) { REGISTER_TEST(creates_correct_number_states); }
