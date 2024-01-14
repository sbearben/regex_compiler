#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "sregex.h"
#include "test_file.h"

TEST_CASE(regex_accepts_matches_exactly) {
   // First pattern
   regex_t* regex = new_regex("(a|b)*ab(b|cc)kkws*");

   assert_true(regex_accepts(regex, "abcckkws"));
   assert_true(regex_accepts(regex, "abababbkkws"));
   assert_true(regex_accepts(regex, "abcckkw"));
   assert_true(regex_accepts(regex, "aaaaabbbbbbbabbkkwsssssss"));

   assert_false(regex_accepts(regex, "abkkw"));
   assert_false(regex_accepts(regex, "abkkwss"));
   assert_false(regex_accepts(regex, "abckkw"));
   assert_false(regex_accepts(regex, "abckkwss"));

   regex_release(regex);

   // Second pattern
   regex = new_regex("a*b*c*");

   assert_true(regex_accepts(regex, ""));
   assert_true(regex_accepts(regex, "a"));
   assert_true(regex_accepts(regex, "b"));
   assert_true(regex_accepts(regex, "c"));
   assert_true(regex_accepts(regex, "ab"));
   assert_true(regex_accepts(regex, "ac"));
   assert_true(regex_accepts(regex, "bc"));
   assert_true(regex_accepts(regex, "abc"));
   assert_true(regex_accepts(regex, "abcc"));
   assert_true(regex_accepts(regex, "aaaccc"));
   assert_true(regex_accepts(regex, "aaabbccc"));

   assert_false(regex_accepts(regex, "d"));
   assert_false(regex_accepts(regex, "ad"));
   assert_false(regex_accepts(regex, "bd"));
   assert_false(regex_accepts(regex, "cd"));
   assert_false(regex_accepts(regex, "abd"));

   regex_release(regex);

   // Third pattern
   regex = new_regex("hello( world| there| you)*");

   assert_true(regex_accepts(regex, "hello world"));
   assert_true(regex_accepts(regex, "hello there"));
   assert_true(regex_accepts(regex, "hello you"));
   assert_true(regex_accepts(regex, "hello"));
   assert_true(regex_accepts(regex, "hello world there world you you"));

   assert_false(regex_accepts(regex, "hello world  there"));
   assert_false(regex_accepts(regex, "hello "));
   assert_false(regex_accepts(regex, "he hello world you"));

   regex_release(regex);
}

TEST_CASE(regex_matches_quantifiers) {
   // First
   regex_t* regex = new_regex("a*b+c?d");

   assert_true(regex_accepts(regex, "abd"));
   assert_true(regex_accepts(regex, "bcd"));
   assert_true(regex_accepts(regex, "bd"));
   assert_true(regex_accepts(regex, "bbbbbbcd"));
   assert_true(regex_accepts(regex, "abbd"));
   assert_true(regex_accepts(regex, "aaaabbbd"));
   assert_true(regex_accepts(regex, "abbbcd"));
   assert_true(regex_accepts(regex, "abbbd"));
   assert_true(regex_accepts(regex, "abcd"));

   assert_false(regex_accepts(regex, "ad"));
   assert_false(regex_accepts(regex, "ac"));
   assert_false(regex_accepts(regex, "ab"));
   assert_false(regex_accepts(regex, "acd"));

   regex_release(regex);

   // Second
   regex = new_regex("hello( world| there| you)?");

   assert_true(regex_accepts(regex, "hello world"));
   assert_true(regex_accepts(regex, "hello there"));
   assert_true(regex_accepts(regex, "hello you"));
   assert_true(regex_accepts(regex, "hello"));

   assert_false(regex_accepts(regex, "hello world there"));

   regex_release(regex);
}

TEST_CASE(regex_test_matches_any_substring) {
   regex_t* regex = new_regex("foo+");

   assert_true(regex_test(regex, "table football"));
   assert_true(regex_test(regex, "food"));
   assert_true(regex_test(regex, "ur a foodie"));
   assert_true(regex_test(regex, "the town fool"));

   assert_false(regex_test(regex, "fo"));
   assert_false(regex_test(regex, "forage"));
   assert_false(regex_test(regex, "look over there"));
   assert_false(regex_test(regex, "the forest is full of trees"));

   regex_release(regex);
}

void on_register_tests(void) {
   REGISTER_TEST(regex_accepts_matches_exactly);
   REGISTER_TEST(regex_matches_quantifiers);
   REGISTER_TEST(regex_test_matches_any_substring);
}
