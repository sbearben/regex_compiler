# regex_compiler

Compile simple regex's using NFA/DFA conversion. This is nothing but a toy created as a learning experience for myself.

## Supported syntax

| Characters    | Meaning |
| -------- | ------- |
| ab  | Concatenation    |
| a\|b | Disjunction     |
| (a) | Grouping     |
| a*    | Matches preceding item 0 or more times |
| a+    | Matches preceding item 1 or more times |
| a?    | Matches preceding item 0 or 1 times |
| .     | Matches any single character except line terminators: \n, \r |
| [abc]     | Matches any of the enclosed characters |
| [^abc]     | A negated character class. Matches any character not enclosed in the brackets |
| [a-m]     | Matches a range of characters |
| \d     | Matches any digit (equivalent to [0-9]) |
| \D     | Matches any character that is not a digit (equivalent to [^0-9]) |
| \w     | Matches any alphanumeric character (equivalent to [A-Za-z0-9_]) |
| \W     | Matches any character that is not an alphanumeric character (equivalent to [^A-Za-z0-9_]) |
| \s     | Matches a single white space character, including space, tab, form feed, line feed |
| \S     | Matches a single character other than white space |

## Resources used for implementation

- Compiler Construction: Principles and Practice (Louden)
- Compilers: Principles, Techniques, and Tools (Aho, Lam, Sethi, Ullman)
