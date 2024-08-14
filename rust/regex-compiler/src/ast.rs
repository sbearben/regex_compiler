use crate::alphabet::Alphabet;
use std::cell::RefCell;
use std::{
    collections::{HashSet, VecDeque},
    vec,
};

#[derive(Debug, Eq, PartialEq)]
pub enum AstNode {
    Alternation(Box<Alternation>),
    Concat(Box<Concat>),
    Repetition(Box<Repetition>),
    Dot,
    CharacterClass(Box<CharacterClass>),
    ClassBracketed(Box<ClassBracketed>),
    Literal(Box<Literal>),
}

// Represents a topological ordering of the AST
#[derive(Debug)]
pub struct AstTopo {
    pub elems: Vec<AstNodeLayer<AstIdx>>,
}

// Can be used to convert recursive AstNodes into a type that is generic in its child types (ex: reference by id)
#[derive(Debug)]
pub enum AstNodeLayer<A> {
    Alternation { left: A, right: A },
    Concat { left: A, right: A },
    Repetition { kind: RepetitionKind, child: A },
    Dot,
    CharacterClass(CharacterClass),
    ClassBracketed(ClassBracketed),
    Literal { value: char },
}

#[derive(Debug)]
pub struct AstIdx(pub usize);

#[derive(Debug, Eq, PartialEq)]
pub struct Alternation {
    pub left: Box<AstNode>,
    pub right: Box<AstNode>,
}

#[derive(Debug, Eq, PartialEq)]
pub struct Concat {
    pub left: Box<AstNode>,
    pub right: Box<AstNode>,
}

#[derive(Debug, Eq, PartialEq)]
pub struct Repetition {
    kind: RepetitionKind,
    child: Box<AstNode>,
}

#[derive(Debug, Eq, PartialEq, Clone)]
pub enum RepetitionKind {
    ZeroOrOne,
    ZeroOrMore,
    OneOrMore,
}

#[derive(Debug, Eq, PartialEq, Clone)]
pub struct CharacterClass {
    kind: CharacterClassKind,
    negated: bool,
}

#[derive(Debug, Eq, PartialEq, Clone)]
pub enum CharacterClassKind {
    Digit,
    Word,
    Whitespace,
}

#[derive(Debug, Eq, PartialEq, Clone)]
pub struct ClassBracketed {
    negated: bool,
    items: Vec<ClassSetItem>,
    computed_characters: RefCell<Option<HashSet<char>>>,
}

#[derive(Debug, Eq, PartialEq, Clone)]
pub enum ClassSetItem {
    Literal(Box<Literal>),
    Range(Box<ClassSetRange>),
    CharacterClass(Box<CharacterClass>),
}

#[derive(Debug, Eq, PartialEq, Clone)]
pub struct ClassSetRange {
    start: Literal,
    end: Literal,
}

#[derive(Debug, Eq, PartialEq, Clone)]
pub struct Literal {
    value: char,
}

// For nodes that can be trivially collapsed into the character set that they accept
pub trait ComputeCharacters {
    fn compute_characters(&self) -> HashSet<char>;
}

// Ast parsing errors
#[derive(Debug)]
pub enum Error {
    // exepected, actual
    UnexpectedToken(char, char),
    UnexpectedEndOfInput,
    InvalidCharacters(Vec<char>),
}

impl AstNode {
    // Static functions

    pub fn alternation(e: Alternation) -> AstNode {
        AstNode::Alternation(Box::new(e))
    }

    pub fn concat(e: Concat) -> AstNode {
        AstNode::Concat(Box::new(e))
    }

    pub fn repetition(e: Repetition) -> AstNode {
        AstNode::Repetition(Box::new(e))
    }

    pub fn dot() -> AstNode {
        AstNode::Dot
    }

    pub fn character_class(e: CharacterClass) -> AstNode {
        AstNode::CharacterClass(Box::new(e))
    }

    pub fn class_bracketed(e: ClassBracketed) -> AstNode {
        AstNode::ClassBracketed(Box::new(e))
    }

    pub fn literal(e: Literal) -> AstNode {
        AstNode::Literal(Box::new(e))
    }
}

impl AstTopo {
    pub fn from_boxed(root: &AstNode) -> Self {
        Self::expand_layers(root, |node| match node {
            AstNode::Alternation(alternation) => AstNodeLayer::Alternation {
                left: &alternation.left,
                right: &alternation.right,
            },
            AstNode::Concat(concat) => AstNodeLayer::Concat {
                left: &concat.left,
                right: &concat.right,
            },
            AstNode::Repetition(repetition) => AstNodeLayer::Repetition {
                kind: repetition.kind.clone(),
                child: &repetition.child,
            },
            AstNode::Dot => AstNodeLayer::Dot,
            AstNode::CharacterClass(character_class) => {
                AstNodeLayer::CharacterClass((**character_class).clone())
            }
            AstNode::ClassBracketed(class_bracketed) => {
                AstNodeLayer::ClassBracketed((**class_bracketed).clone())
            }
            AstNode::Literal(literal) => AstNodeLayer::Literal {
                value: literal.value,
            },
        })
    }

    fn expand_layers<A, F: Fn(A) -> AstNodeLayer<A>>(root: A, expand_layer: F) -> Self {
        let mut frontier = VecDeque::from([root]);
        let mut elems = vec![];

        while let Some(node) = frontier.pop_front() {
            let layer = expand_layer(node);

            let layer = layer.map(|node| {
                frontier.push_back(node);
                // idx of pointed to element
                AstIdx(elems.len() + frontier.len())
            });

            elems.push(layer);
        }

        AstTopo { elems }
    }
}

impl<A> AstNodeLayer<A> {
    pub fn map<F: FnMut(A) -> B, B>(self, mut f: F) -> AstNodeLayer<B> {
        match self {
            AstNodeLayer::Alternation { left, right } => AstNodeLayer::Alternation {
                left: f(left),
                right: f(right),
            },
            AstNodeLayer::Concat { left, right } => AstNodeLayer::Concat {
                left: f(left),
                right: f(right),
            },
            AstNodeLayer::Repetition { kind, child } => AstNodeLayer::Repetition {
                kind,
                child: f(child),
            },
            AstNodeLayer::Dot => AstNodeLayer::Dot,
            AstNodeLayer::CharacterClass(character_class) => {
                AstNodeLayer::CharacterClass(character_class)
            }
            AstNodeLayer::ClassBracketed(class_bracketed) => {
                AstNodeLayer::ClassBracketed(class_bracketed)
            }
            AstNodeLayer::Literal { value } => AstNodeLayer::Literal { value },
        }
    }
}

impl Alternation {
    pub fn new(left: AstNode, right: AstNode) -> Self {
        Alternation {
            left: Box::new(left),
            right: Box::new(right),
        }
    }
}

impl Concat {
    pub fn new(left: AstNode, right: AstNode) -> Self {
        Concat {
            left: Box::new(left),
            right: Box::new(right),
        }
    }
}

impl Repetition {
    pub fn new(kind: RepetitionKind, child: AstNode) -> Self {
        Repetition {
            kind,
            child: Box::new(child),
        }
    }
}

impl CharacterClass {
    pub fn new(kind: CharacterClassKind, negated: bool) -> CharacterClass {
        CharacterClass { kind, negated }
    }
}

impl ComputeCharacters for CharacterClass {
    fn compute_characters(&self) -> HashSet<char> {
        match self.kind {
            CharacterClassKind::Digit => Alphabet::digit_characters(self.negated),
            CharacterClassKind::Word => Alphabet::word_characters(self.negated),
            CharacterClassKind::Whitespace => Alphabet::whitespace_characters(self.negated),
        }
    }
}

impl ClassBracketed {
    pub fn new(negated: bool) -> Self {
        ClassBracketed {
            negated,
            items: vec![],
            computed_characters: RefCell::new(None),
        }
    }

    pub fn add_literal_item(&mut self, literal: Literal) {
        self.items.push(ClassSetItem::Literal(Box::new(literal)))
    }

    pub fn add_character_class_item(&mut self, cc: CharacterClass) {
        self.items.push(ClassSetItem::CharacterClass(Box::new(cc)))
    }

    pub fn add_range_item(&mut self, range: ClassSetRange) {
        self.items.push(ClassSetItem::Range(Box::new(range)))
    }
}

impl ComputeCharacters for ClassBracketed {
    fn compute_characters(&self) -> HashSet<char> {
        if let Some(characters) = &*self.computed_characters.borrow() {
            return characters.clone();
        }

        let mut characters = HashSet::new();

        for item in &self.items {
            match item {
                ClassSetItem::Literal(literal) => {
                    characters.insert(literal.value);
                }
                ClassSetItem::CharacterClass(character_class) => {
                    let character_set = character_class.compute_characters();
                    characters.extend(character_set.iter());
                }
                ClassSetItem::Range(range) => {
                    let start = range.start.value as u32;
                    let end = range.end.value as u32;
                    for i in start..=end {
                        characters.insert(i as u8 as char);
                    }
                }
            }
        }

        if self.negated {
            let all_characters = Alphabet::all_characters();
            characters = all_characters.difference(&characters).cloned().collect();
        }

        self.computed_characters.replace(Some(characters));
        self.computed_characters.borrow().clone().unwrap()
    }
}

impl ClassSetRange {
    pub fn new(start: Literal, end: Literal) -> Self {
        ClassSetRange { start, end }
    }
}

impl Literal {
    pub fn new(value: char) -> Self {
        Literal { value }
    }
}
