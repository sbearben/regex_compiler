use std::collections::{HashMap, HashSet};
use std::sync::OnceLock;

type AlphabetConfig = HashMap<char, [u8; 2]>;

pub struct Alphabet {
    map: &'static AlphabetConfig,
}

impl Alphabet {
    // Associated methods

    pub fn new() -> Alphabet {
        static LANG_MAP: OnceLock<AlphabetConfig> = OnceLock::new();

        Alphabet {
            map: LANG_MAP.get_or_init(|| ALPHABET_CONFIG_ENTRIES.iter().cloned().collect()),
        }
    }

    pub fn all_characters() -> HashSet<char> {
        static ALL_CHARS_SET: OnceLock<HashSet<char>> = OnceLock::new();
        ALL_CHARS_SET
            .get_or_init(|| ALPHABET_CONFIG_ENTRIES.iter().map(|(c, _)| *c).collect())
            .clone()
    }

    pub fn whitespace_characters(negated: bool) -> HashSet<char> {
        static WHITESPACE_CHARS_SET: OnceLock<HashSet<char>> = OnceLock::new();
        static WHITESPACE_CHARS_SET_NEGATED: OnceLock<HashSet<char>> = OnceLock::new();

        Alphabet::get_or_init_character_sets(
            WHITESPACE_CHARACTERS,
            &WHITESPACE_CHARS_SET,
            &WHITESPACE_CHARS_SET_NEGATED,
            negated,
        )
    }

    pub fn digit_characters(negated: bool) -> HashSet<char> {
        static DIGIT_CHARS_SET: OnceLock<HashSet<char>> = OnceLock::new();
        static DIGIT_CHARS_SET_NEGATED: OnceLock<HashSet<char>> = OnceLock::new();

        Alphabet::get_or_init_character_sets(
            DIGIT_CHARACTERS,
            &DIGIT_CHARS_SET,
            &DIGIT_CHARS_SET_NEGATED,
            negated,
        )
    }

    pub fn word_characters(negated: bool) -> HashSet<char> {
        static WORD_CHARS_SET: OnceLock<HashSet<char>> = OnceLock::new();
        static WORD_CHARS_SET_NEGATED: OnceLock<HashSet<char>> = OnceLock::new();

        Alphabet::get_or_init_character_sets(
            WORD_CHARACTERS,
            &WORD_CHARS_SET,
            &WORD_CHARS_SET_NEGATED,
            negated,
        )
    }

    fn get_or_init_character_sets(
        characters: &'static [char],
        base_set_container: &'static OnceLock<HashSet<char>>,
        negated_set_container: &'static OnceLock<HashSet<char>>,
        negated: bool,
    ) -> HashSet<char> {
        if negated {
            negated_set_container
                .get_or_init(|| {
                    ALPHABET_CONFIG_ENTRIES
                        .iter()
                        .map(|(c, _)| *c)
                        .filter(|c| !characters.contains(c))
                        .collect()
                })
                .clone()
        } else {
            base_set_container
                .get_or_init(|| characters.iter().cloned().collect())
                .clone()
        }
    }

    // Instance methods

    pub fn is_valid_character(&self, value: &char) -> bool {
        self.map.contains_key(value)
    }

    pub fn is_special_character(&self, value: &char) -> bool {
        self.map.get(value).unwrap()[0] == 1
    }

    pub fn is_quantifier_symbol(&self, value: &char) -> bool {
        self.map.get(value).unwrap()[1] == 1
    }
}
// (is_special, is_quantifier)
const ALPHABET_CONFIG_ENTRIES: &[(char, [u8; 2])] = &[
    ('\t', [0, 0]),
    ('\n', [0, 0]),
    ('\r', [0, 0]),
    (' ', [0, 0]),
    ('!', [0, 0]),
    ('"', [1, 0]),
    ('#', [0, 0]),
    ('$', [0, 0]),
    ('%', [0, 0]),
    ('&', [0, 0]),
    ('\'', [0, 0]),
    ('(', [1, 0]),
    (')', [1, 0]),
    ('*', [1, 1]),
    ('+', [1, 1]),
    (',', [0, 0]),
    ('-', [0, 0]),
    ('.', [1, 0]),
    ('/', [0, 0]),
    ('0', [0, 0]),
    ('1', [0, 0]),
    ('2', [0, 0]),
    ('3', [0, 0]),
    ('4', [0, 0]),
    ('5', [0, 0]),
    ('6', [0, 0]),
    ('7', [0, 0]),
    ('8', [0, 0]),
    ('9', [0, 0]),
    (':', [0, 0]),
    (';', [0, 0]),
    ('<', [0, 0]),
    ('=', [0, 0]),
    ('>', [0, 0]),
    ('?', [1, 1]),
    ('@', [0, 0]),
    ('A', [0, 0]),
    ('B', [0, 0]),
    ('C', [0, 0]),
    ('D', [0, 0]),
    ('E', [0, 0]),
    ('F', [0, 0]),
    ('G', [0, 0]),
    ('H', [0, 0]),
    ('I', [0, 0]),
    ('J', [0, 0]),
    ('K', [0, 0]),
    ('L', [0, 0]),
    ('M', [0, 0]),
    ('N', [0, 0]),
    ('O', [0, 0]),
    ('P', [0, 0]),
    ('Q', [0, 0]),
    ('R', [0, 0]),
    ('S', [0, 0]),
    ('T', [0, 0]),
    ('U', [0, 0]),
    ('V', [0, 0]),
    ('W', [0, 0]),
    ('X', [0, 0]),
    ('Y', [0, 0]),
    ('Z', [0, 0]),
    ('[', [1, 0]),
    ('\\', [1, 0]),
    (']', [1, 0]),
    ('^', [1, 0]),
    ('_', [0, 0]),
    ('`', [0, 0]),
    ('a', [0, 0]),
    ('b', [0, 0]),
    ('c', [0, 0]),
    ('d', [0, 0]),
    ('e', [0, 0]),
    ('f', [0, 0]),
    ('g', [0, 0]),
    ('h', [0, 0]),
    ('i', [0, 0]),
    ('j', [0, 0]),
    ('k', [0, 0]),
    ('l', [0, 0]),
    ('m', [0, 0]),
    ('n', [0, 0]),
    ('o', [0, 0]),
    ('p', [0, 0]),
    ('q', [0, 0]),
    ('r', [0, 0]),
    ('s', [0, 0]),
    ('t', [0, 0]),
    ('u', [0, 0]),
    ('v', [0, 0]),
    ('w', [0, 0]),
    ('x', [0, 0]),
    ('y', [0, 0]),
    ('z', [0, 0]),
    ('{', [0, 0]),
    ('|', [1, 0]),
    ('}', [0, 0]),
    ('~', [0, 0]),
];

const WHITESPACE_CHARACTERS: &[char] = &[' ', '\t', '\n', '\r'];

const DIGIT_CHARACTERS: &[char] = &['0', '1', '2', '3', '4', '5', '6', '7', '8', '9'];

const WORD_CHARACTERS: &[char] = &[
    'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's',
    't', 'u', 'v', 'w', 'x', 'y', 'z', 'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L',
    'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z', '0', '1', '2', '3', '4',
    '5', '6', '7', '8', '9', '0', '_',
];
