use crate::dfa::DFA;
use crate::nfa::NFA;
use crate::parse::Parser;

pub struct Regex {
    dfa: DFA,
}

impl Regex {
    pub fn new(pattern: &str) -> Self {
        let ast = Parser::parse(pattern).unwrap();
        let nfa = NFA::from_ast(&ast);
        let dfa = DFA::from_nfa(&nfa);

        Regex { dfa }
    }

    pub fn accepts(&self, input: &str) -> bool {
        self.dfa.accepts(input)
    }

    // Returns true if the provided string contains any substring that matches the regex.
    pub fn test(&self, input: &str) -> bool {
        let mut start = 0;
        let end = input.len();
        let mut forward: usize;

        while start < end {
            forward = start + 1;
            while forward <= end {
                if self.accepts(&input[start..forward]) {
                    return true;
                }
                forward += 1;
            }
            start += 1;
        }
        false
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn it_accepts_matches_exactly() {
        let regex = Regex::new("(a|b)*ab(b|cc)kkws*");

        assert_eq!(regex.accepts("abcckkws"), true);
        assert_eq!(regex.accepts("abababbkkws"), true);
        assert_eq!(regex.accepts("abcckkw"), true);
        assert_eq!(regex.accepts("aaaaabbbbbbbabbkkwsssssss"), true);

        assert_eq!(regex.accepts("abkkw"), false);
        assert_eq!(regex.accepts("abkkwss"), false);
        assert_eq!(regex.accepts("abckkw"), false);
        assert_eq!(regex.accepts("abckkwss"), false);

        let regex = Regex::new("a*b*c*");

        assert_eq!(regex.accepts(""), true);
        assert_eq!(regex.accepts("a"), true);
        assert_eq!(regex.accepts("b"), true);
        assert_eq!(regex.accepts("c"), true);
        assert_eq!(regex.accepts("ab"), true);
        assert_eq!(regex.accepts("ac"), true);
        assert_eq!(regex.accepts("bc"), true);
        assert_eq!(regex.accepts("abc"), true);
        assert_eq!(regex.accepts("abcc"), true);
        assert_eq!(regex.accepts("aaaccc"), true);
        assert_eq!(regex.accepts("aaabbccc"), true);

        assert_eq!(regex.accepts("d"), false);
        assert_eq!(regex.accepts("ad"), false);
        assert_eq!(regex.accepts("bd"), false);
        assert_eq!(regex.accepts("cd"), false);
        assert_eq!(regex.accepts("abd"), false);

        let regex = Regex::new("hello( world| there| you)*");

        assert_eq!(regex.accepts("hello world"), true);
        assert_eq!(regex.accepts("hello there"), true);
        assert_eq!(regex.accepts("hello you"), true);
        assert_eq!(regex.accepts("hello"), true);
        assert_eq!(regex.accepts("hello world there world you you"), true);

        assert_eq!(regex.accepts("hello world  there"), false);
        assert_eq!(regex.accepts("hello "), false);
        assert_eq!(regex.accepts("he hello world you"), false);
    }

    #[test]
    fn it_matches_quantifiers() {
        // First
        let regex = Regex::new("a*b+c?d");

        assert_eq!(regex.accepts("abd"), true);
        assert_eq!(regex.accepts("bcd"), true);
        assert_eq!(regex.accepts("bd"), true);
        assert_eq!(regex.accepts("bbbbbbcd"), true);
        assert_eq!(regex.accepts("abbd"), true);
        assert_eq!(regex.accepts("aaaabbbd"), true);
        assert_eq!(regex.accepts("abbbcd"), true);
        assert_eq!(regex.accepts("abbbd"), true);
        assert_eq!(regex.accepts("abcd"), true);

        assert_eq!(regex.accepts("ad"), false);
        assert_eq!(regex.accepts("ac"), false);
        assert_eq!(regex.accepts("ab"), false);
        assert_eq!(regex.accepts("acd"), false);

        // Second
        let regex = Regex::new("hello( world| there| you)?");

        assert_eq!(regex.accepts("hello world"), true);
        assert_eq!(regex.accepts("hello there"), true);
        assert_eq!(regex.accepts("hello you"), true);
        assert_eq!(regex.accepts("hello"), true);

        assert_eq!(regex.accepts("hello world there"), false);
    }

    #[test]
    fn it_matches_any_substring() {
        let regex = Regex::new("foo+");

        assert_eq!(regex.test("table football"), true);
        assert_eq!(regex.test("food"), true);
        assert_eq!(regex.test("ur a foodie"), true);
        assert_eq!(regex.test("the town fool"), true);

        assert_eq!(regex.test("fo"), false);
        assert_eq!(regex.test("forage"), false);
        assert_eq!(regex.test("look over there"), false);
        assert_eq!(regex.test("the forest is full of trees"), false);
    }

    #[test]
    fn it_matches_escape_characters() {
        // First
        let regex = Regex::new("they're \\(\\\"them\\\"\\)\\.");

        assert_eq!(regex.accepts("they're (\"them\")."), true);
        assert_eq!(regex.accepts("they're (them)"), false);

        // Second
        let regex = Regex::new(r"2005 cup champions\*");
        assert_eq!(regex.accepts("2005 cup champions*"), true);

        // Third
        let regex = Regex::new(r"how are you\?");
        assert_eq!(regex.accepts("how are you?"), true);
    }

    #[test]
    fn it_works_with_any_character_class() {
        // First
        let regex = Regex::new(r"(hey )?do you like foo.*\?");

        assert_eq!(regex.accepts("hey do you like foo?"), true);
        assert_eq!(regex.accepts("do you like foo?"), true);
        assert_eq!(regex.accepts("do you like food?"), true);
        assert_eq!(regex.accepts("do you like football?"), true);
        assert_eq!(regex.accepts("hey do you like food and eating out?"), true);

        // Second
        let regex = Regex::new("import \\{.*,? doThis.* \\} from \\\"some-package\\\";");

        assert_eq!(
            regex.accepts("import { doThis } from \"some-package\";"),
            true
        );
        assert_eq!(
            regex.accepts("import { doThis, doThat } from \"some-package\";"),
            true
        );
        assert_eq!(
            regex.accepts("import { doOther, doThis, doThat } from \"some-package\";"),
            true
        );

        assert_eq!(
            regex.accepts("import { doThat } from \"some-package\""),
            false
        );
        assert_eq!(
            regex.accepts("import { doThat, doOther } from \"some-package\""),
            false
        );
    }

    #[test]
    fn it_works_with_character_ranges() {
        // First
        let regex = Regex::new(r"[a-z]+( [a-z]+)*\.?");

        assert_eq!(regex.accepts("hello"), true);
        assert_eq!(regex.accepts("hello world"), true);
        assert_eq!(regex.accepts("i am writing a sentence."), true);

        assert_eq!(regex.accepts("I am writing a sentence."), false);
        assert_eq!(regex.accepts("HELLO"), false);
        assert_eq!(regex.accepts("HELLO WORLD"), false);

        // Second
        let regex = Regex::new(r"[a-zA-Z][a-zA-Z0-9_]*");

        assert_eq!(regex.accepts("hello"), true);
        assert_eq!(regex.accepts("Hello"), true);
        assert_eq!(regex.accepts("hello_world"), true);
        assert_eq!(regex.accepts("hello_world_123"), true);

        assert_eq!(regex.accepts("hello world"), false);
        assert_eq!(regex.accepts("1hello_world_123"), false);

        // Third
        let regex = Regex::new(r"[^abc][^a-z]*");

        assert_eq!(regex.accepts("dA0"), true);
        assert_eq!(regex.accepts("dA0!@#$%^&*()_+"), true);
        assert_eq!(regex.accepts("0000AAAAA"), true);

        assert_eq!(regex.accepts("abc"), false);
        assert_eq!(regex.accepts("zbba"), false);

        // Fourth
        let regex = Regex::new(r"[\d-\w]+");

        assert_eq!(regex.accepts("hello2233-"), true);
        assert_eq!(regex.accepts("-99kjakAA--"), true);

        assert_eq!(regex.accepts("hello world"), false);
    }

    #[test]
    fn it_matches_tabs_and_newlines() {
        let regex = Regex::new("hello\n?\tworld");

        assert_eq!(regex.accepts("hello\n\tworld"), true);
        assert_eq!(regex.accepts("hello\tworld"), true);
    }

    #[test]
    fn it_matches_character_classes() {
        // First
        let regex = Regex::new("\\w+\\s+\\w+");

        assert_eq!(regex.accepts("hello world"), true);
        assert_eq!(regex.accepts("bob \t35"), true);
        assert_eq!(regex.accepts("askj7837jj\n  \t111ss"), true);

        assert_eq!(regex.accepts(" howdy"), false);
        assert_eq!(regex.accepts("?hey there"), false);

        // Second
        let regex = Regex::new(r"\d+\s+\d+");

        assert_eq!(regex.accepts("123 456"), true);
        assert_eq!(regex.accepts("1\t  2"), true);
        assert_eq!(regex.accepts("99   \n\t\r 2"), true);

        assert_eq!(regex.accepts("hey there"), false);
        assert_eq!(regex.accepts("123 456 789"), false);

        // Third
        let regex = Regex::new(r"\W+");

        assert_eq!(regex.accepts(r"\?!,.\(\)[]{}"), true);
        assert_eq!(regex.accepts("hello"), false);

        // Fourth
        let regex = Regex::new(r"\S+");

        assert_eq!(regex.accepts("k"), true);
        // assert_eq!(regex.accepts("hello\tworld"), false);

        // Fifth
        let regex = Regex::new(r"\D+");

        assert_eq!(regex.accepts("hello"), true);
        assert_eq!(regex.accepts("hello world"), true);
        assert_eq!(regex.accepts("123"), false);
    }
}
