use crate::alphabet::Alphabet;
use crate::ast::{self, AstNode, Literal};
use std::cell::Cell;

type Result<T> = core::result::Result<T, ast::Error>;

pub struct Parser {
    pattern: String,
    offset: Cell<usize>,
    alphabet: Alphabet,
}

impl Parser {
    pub fn parse(pattern: &str) -> Result<AstNode> {
        let lang = Alphabet::new();
        if let Err(error) = Parser::validate_pattern_alphabet(&lang, pattern) {
            panic!("Validation error: {:?}", error);
        }
        let parser = Parser {
            pattern: pattern.to_owned(),
            offset: Cell::new(0),
            alphabet: lang,
        };

        parser.parse_regexp()
    }

    fn validate_pattern_alphabet(lang: &Alphabet, pattern: &str) -> Result<()> {
        let invalid_chars: Vec<char> = pattern
            .chars()
            .filter(|c| !lang.is_valid_character(c))
            .collect();
        if invalid_chars.is_empty() {
            Ok(())
        } else {
            Err(ast::Error::InvalidCharacters(invalid_chars))
        }
    }

    fn parse_regexp(&self) -> Result<AstNode> {
        let mut temp = self.parse_concatenation()?;
        while let Some(_) = self.match_char('|') {
            temp = AstNode::alternation(ast::Alternation::new(temp, self.parse_concatenation()?));
        }
        Ok(temp)
    }

    fn parse_concatenation(&self) -> Result<AstNode> {
        let mut temp = self.parse_quantifier()?;
        while self
            .peek()
            .filter(|c| self.in_factor_first_set(c))
            .is_some()
        {
            // We don't match here since current token is part of first set of `factor`, so if we matched the conditions in factor will fail
            temp = AstNode::concat(ast::Concat::new(temp, self.parse_quantifier()?))
        }
        Ok(temp)
    }

    fn parse_quantifier(&self) -> Result<AstNode> {
        let temp = self.parse_factor()?;
        if let Some(current_char) = self.match_cond(|c| self.alphabet.is_quantifier_symbol(c)) {
            let kind: ast::RepetitionKind = match current_char {
                '*' => ast::RepetitionKind::ZeroOrMore,
                '+' => ast::RepetitionKind::OneOrMore,
                '?' => ast::RepetitionKind::ZeroOrOne,
                _ => unreachable!(),
            };
            Ok(AstNode::repetition(ast::Repetition::new(kind, temp)))
        } else {
            Ok(temp)
        }
    }

    fn parse_factor(&self) -> Result<AstNode> {
        if let Some(current_char) = self.next() {
            match current_char {
                '(' => {
                    let factor = self.parse_regexp()?;
                    self.match_char(')').unwrap();
                    return Ok(factor);
                }
                '\\' => {
                    let value = self.next().unwrap();
                    if let Some(character_class) = derive_character_class(&value) {
                        return Ok(AstNode::character_class(character_class));
                    } else {
                        return Ok(AstNode::literal(Literal::new(value)));
                    }
                }
                x if !self.alphabet.is_special_character(&x) => {
                    return Ok(AstNode::literal(Literal::new(current_char)));
                }
                '.' => {
                    return Ok(AstNode::dot());
                }
                '[' => {
                    let factor = self.parse_class_bracketed()?;
                    self.match_char(']').unwrap();
                    return Ok(factor);
                }
                _ => return Err(ast::Error::UnexpectedToken('\0', current_char)),
            }
        }
        Err(ast::Error::UnexpectedEndOfInput)
    }

    fn parse_class_bracketed(&self) -> Result<AstNode> {
        let negated = self.match_char('^').is_some();
        let mut class_bracketed = ast::ClassBracketed::new(negated);

        while let Some(start) = self.match_cond(|c| *c != ']') {
            if start == '\\' {
                let value = self.next().unwrap();
                if let Some(character_class) = derive_character_class(&value) {
                    class_bracketed.add_character_class_item(character_class);
                } else {
                    class_bracketed.add_literal_item(Literal::new(value));
                }
                continue;
            }

            if self.match_char('-').is_some() {
                let end = self.next().unwrap();
                if start > end {
                    continue;
                }
                class_bracketed.add_range_item(ast::ClassSetRange::new(
                    Literal::new(start),
                    Literal::new(end),
                ));
            } else {
                class_bracketed.add_literal_item(Literal::new(start));
            }
        }

        Ok(AstNode::class_bracketed(class_bracketed))
    }

    pub fn in_factor_first_set(&self, value: &char) -> bool {
        return !self.alphabet.is_special_character(value)
            || *value == '('
            || *value == '\\'
            || *value == '.'
            || *value == '[';
    }

    fn peek(&self) -> Option<char> {
        self.pattern.chars().nth(self.offset.get())
    }

    // Consumes the current character if it matches what's expected, returns None if it doesn't
    fn match_char(&self, expected: char) -> Option<char> {
        self.match_cond(|&current| current == expected)
    }

    // Consumes the current character if it matches the condition, returns None if it doesn't
    fn match_cond<P>(&self, predicate: P) -> Option<char>
    where
        P: FnOnce(&char) -> bool,
    {
        self.peek().filter(predicate).map(|current| {
            self.offset.set(self.offset.get() + 1);
            current
        })
    }

    fn next(&self) -> Option<char> {
        let current = self.peek()?;
        self.offset.set(self.offset.get() + 1);
        Some(current)
    }
}

fn derive_character_class(value: &char) -> Option<ast::CharacterClass> {
    match value {
        'd' => Some(ast::CharacterClass::new(
            ast::CharacterClassKind::Digit,
            false,
        )),
        'D' => Some(ast::CharacterClass::new(
            ast::CharacterClassKind::Digit,
            true,
        )),
        'w' => Some(ast::CharacterClass::new(
            ast::CharacterClassKind::Word,
            false,
        )),
        'W' => Some(ast::CharacterClass::new(
            ast::CharacterClassKind::Word,
            true,
        )),
        's' => Some(ast::CharacterClass::new(
            ast::CharacterClassKind::Whitespace,
            false,
        )),
        'S' => Some(ast::CharacterClass::new(
            ast::CharacterClassKind::Whitespace,
            true,
        )),
        _ => None,
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn it_works() {
        let ast = Parser::parse("a").unwrap();
        assert_eq!(ast, AstNode::literal(Literal::new('a')));
    }

    #[test]
    fn it_works_with_concatenation() {
        let ast = Parser::parse("ab").unwrap();
        assert_eq!(
            ast,
            AstNode::concat(ast::Concat::new(
                AstNode::literal(Literal::new('a')),
                AstNode::literal(Literal::new('b'))
            ))
        );
    }

    #[test]
    fn it_works_with_alternation() {
        let ast = Parser::parse("a|b").unwrap();
        assert_eq!(
            ast,
            AstNode::alternation(ast::Alternation::new(
                AstNode::literal(Literal::new('a')),
                AstNode::literal(Literal::new('b'))
            ))
        );
    }

    #[test]
    fn it_works_with_quantifiers() {
        let ast = Parser::parse("a*").unwrap();
        assert_eq!(
            ast,
            AstNode::repetition(ast::Repetition::new(
                ast::RepetitionKind::ZeroOrMore,
                AstNode::literal(Literal::new('a'))
            ))
        );
    }
}
