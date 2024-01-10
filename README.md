# regex_compiler

Compile simple regex's using NFA/DFA conversion. This is nothing but a toy created as a learning experience for myself.

## Supported syntax

| Characters    | Meaning |
| -------- | ------- |
| ab  | Concatenation    |
| a\|b | Disjunction     |
| (a) | Grouping     |
| a*    | Matches preceding item 0 or more times    |
| a+    | Matches preceding item 1 or more times    |
| a?    | Matches preceding item 0 or 1 times    |

## Resources used for implementation

- Compiler Construction: Principles and Practice (Louden)
- Compilers: Principles, Techniques, and Tools (Aho, Lam, Sethi, Ullman)
