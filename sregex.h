// sregex (simple regex, not to be confused with structural regex)

#ifndef SREGEX_H
#define SREGEX_H

#include <stdbool.h>

typedef struct regex regex_t;

/**
 * Returns true if the regex accepts the provided string (exact match).
 * @param regex The regex to test
 * @param input The string to test against the regex (null-terminated)
 * @return true if the regex accepts the string, false if it doesn't
*/
bool regex_accepts(regex_t*, char*);

/**
 * Returns true if the provided string contains any substring that matches the regex.
 * @param regex The regex to test
 * @param input The string to test against the regex (null-terminated)
 * @return true if the regex matches any substring in the string, false if it doesn't
*/
bool regex_test(regex_t*, char*);

regex_t* new_regex(char*);
void regex_release(regex_t*);

#endif  // SREGEX_H
