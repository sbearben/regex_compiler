#include "nfa.h"

#include "test_file.h"

TEST_CASE(nfa_has_correct_number_states) {
   ast_node_t* ast;
   nfa_t* nfa;

   ast = parse_regex("a");
   nfa = nfa_from_ast(ast);
   assert_int_equal(nfa_num_states(nfa), 2);
   free_nfa(nfa);
   free_ast(ast);

   ast = parse_regex("ab");
   nfa = nfa_from_ast(ast);
   assert_int_equal(nfa_num_states(nfa), 4);
   free_nfa(nfa);
   free_ast(ast);

   ast = parse_regex("a*");
   nfa = nfa_from_ast(ast);
   assert_int_equal(nfa_num_states(nfa), 4);
   free_nfa(nfa);
   free_ast(ast);

   ast = parse_regex("a|b");
   nfa = nfa_from_ast(ast);
   assert_int_equal(nfa_num_states(nfa), 6);
   free_nfa(nfa);
   free_ast(ast);
}

void on_register_tests(void) { REGISTER_TEST(nfa_has_correct_number_states); }
