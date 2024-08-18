use crate::alphabet::Alphabet;
use crate::ast::{AstNode, AstNodeLayer, AstTopo, ComputeCharacters, RepetitionKind};
use std::collections::{HashMap, HashSet};

struct Builder {
    nodes: Vec<NFANode>,
    character_set: HashSet<char>,
    start: Option<NFANodeIdx>,
}

#[derive(Debug)]
pub struct NFA {
    nodes: Vec<NFANode>,
    start: NFANodeIdx,
    character_set: HashSet<char>,
}

#[derive(Clone, Debug)]
pub struct NFANode {
    pub is_accepting: bool,
    edges: Vec<NFAEdge>,
}

#[derive(Clone, Debug)]
pub enum NFAEdge {
    Epsilon { to: NFANodeIdx },
    Literal { literal: char, to: NFANodeIdx },
}

#[derive(Clone, PartialEq, Eq, Hash, Copy, Debug)]
pub struct NFANodeIdx(usize);

#[derive(PartialEq, Eq, Clone)]
pub struct EpsilonClosure {
    pub id: String,
    pub is_accepting: bool,
    nodes: HashSet<NFANodeIdx>,
}

impl NFA {
    pub fn from_ast(root: &AstNode) -> Self {
        let mut builder = Builder::new();
        builder.from_ast(root).build()
    }

    pub fn accepts(&self, input: &str) -> bool {
        let mut move_set_cache = HashMap::new();
        let mut closure_cache = HashMap::new();

        let mut current = self.epsilon_closure(self.start);
        closure_cache.insert(current.id.clone(), current.clone());

        for c in input.chars() {
            let move_set = move_set_cache
                .entry((current.id.clone(), c))
                .or_insert_with(|| self.compute_move_set(&current, c))
                .clone();

            if (move_set).is_empty() {
                return false;
            }

            current = closure_cache
                .entry(EpsilonClosure::id_for_set(&move_set))
                .or_insert_with(|| self.epsilon_closure_set(move_set))
                .clone();
        }

        current.is_accepting
    }

    pub fn start(&self) -> NFANodeIdx {
        self.start
    }

    pub fn node(&self, idx: NFANodeIdx) -> &NFANode {
        &self.nodes[idx.0]
    }

    pub fn character_set_iter(&self) -> impl Iterator<Item = &char> {
        self.character_set.iter()
    }

    // Returns the epsilon closure of a given NFA node
    pub fn epsilon_closure(&self, node: NFANodeIdx) -> EpsilonClosure {
        self.epsilon_closure_set(HashSet::from([node]))
    }

    // Returns the epsilon closure of a given set of NFA nodes
    pub fn epsilon_closure_set(&self, node_set: HashSet<NFANodeIdx>) -> EpsilonClosure {
        let id = EpsilonClosure::id_for_set(&node_set);
        let mut closure_set = HashSet::new();
        let mut stack: Vec<_> = node_set.into_iter().collect();

        while let Some(idx) = stack.pop() {
            closure_set.insert(idx);

            for edge in &self.node(idx).edges {
                if let NFAEdge::Epsilon { to } = edge {
                    if !closure_set.contains(to) {
                        stack.push(*to);
                    }
                }
            }
        }

        let is_accepting = closure_set.iter().any(|idx| self.node(*idx).is_accepting);

        EpsilonClosure::new(id, closure_set, is_accepting)
    }

    //  Gicen a literal, returns the move set for a given set of NFA nodes.
    pub fn compute_move_set(&self, closure: &EpsilonClosure, literal: char) -> HashSet<NFANodeIdx> {
        let mut move_set = HashSet::new();

        for idx in closure.nodes.iter() {
            if let Some(to) = self.node(*idx).find_transition(literal) {
                move_set.insert(to);
            }
        }

        move_set
    }
}

impl Builder {
    pub fn new() -> Self {
        Builder {
            nodes: vec![],
            character_set: HashSet::new(),
            start: None,
        }
    }

    pub fn build(&self) -> NFA {
        NFA {
            nodes: self.nodes.clone(),
            character_set: self.character_set.clone(),
            start: self.start.unwrap(),
        }
    }

    // 1. Topologically sort the AST
    // 2. "Evaluate" the topologically sorted AST (evaluate here means convert / collapse into an NFA)
    pub fn from_ast(&mut self, root: &AstNode) -> &Self {
        let ast_topo = AstTopo::from_boxed(root);

        // A list that reflects evaluated AstNodes. The evaluation is a 'start' and 'end' index into the 'nodes'
        // vector. This map has the same length and order as the topologically sorted AstNodes (ie, an AstNode's
        // Idx is what we use to index into results_map).
        let mut results_map: Vec<(NFANodeIdx, NFANodeIdx)> =
            Vec::with_capacity(ast_topo.elems.len());

        // TODO: Fix this as it's hacky. Essentially because we're reversing the elements, when we go to write
        // `results_map[idx] = result;` at the end, this ends up being out of bounds.
        for _ in 0..ast_topo.elems.len() {
            results_map.push((NFANodeIdx(0), NFANodeIdx(0)));
        }

        for (idx, layer) in ast_topo.elems.into_iter().enumerate().rev() {
            let layer = layer.map(|idx| results_map[idx.0]);

            // Below here, we:
            // 1. take each one of the ast_nodes
            // 2. create its nfa_nodes and put them in `nodes`
            // 3. take the 'start' and 'end' node indeces and write them to results_map
            let result: (NFANodeIdx, NFANodeIdx) = match layer {
                AstNodeLayer::Alternation { left, right } => {
                    let (left_start, left_end) = left;
                    let (right_start, right_end) = right;
                    let start = self.add_node();
                    let end = self.add_node();
                    self.epsilon_connect(start, left_start);
                    self.epsilon_connect(start, right_start);
                    self.epsilon_connect(left_end, end);
                    self.epsilon_connect(right_end, end);
                    (start, end)
                }
                AstNodeLayer::Concat { left, right } => {
                    let (left_start, left_end) = left;
                    let (right_start, right_end) = right;
                    self.epsilon_connect(left_end, right_start);
                    (left_start, right_end)
                }
                AstNodeLayer::Repetition { kind, child } => {
                    let (child_start, child_end) = child;
                    let start = self.add_node();
                    let end = self.add_node();
                    self.epsilon_connect(start, child_start);
                    self.epsilon_connect(child_end, end);

                    match kind {
                        RepetitionKind::ZeroOrMore => {
                            self.epsilon_connect(child_start, end);
                            self.epsilon_connect(child_end, child_start);
                        }
                        RepetitionKind::OneOrMore => {
                            self.epsilon_connect(child_end, child_start);
                        }
                        RepetitionKind::ZeroOrOne => {
                            self.epsilon_connect(child_start, end);
                        }
                    }

                    (start, end)
                }
                AstNodeLayer::Dot => {
                    let characters = Alphabet::all_characters();
                    self.character_set.extend(characters.iter().cloned());
                    self.add_alteration_system_for_characters(&characters)
                }
                AstNodeLayer::CharacterClass(character_class) => {
                    let characters = character_class.compute_characters();
                    self.character_set.extend(characters.iter().cloned());
                    self.add_alteration_system_for_characters(&characters)
                }
                AstNodeLayer::ClassBracketed(class_bracketed) => {
                    let characters = class_bracketed.compute_characters();
                    self.character_set.extend(characters.iter().cloned());
                    self.add_alteration_system_for_characters(&characters)
                }
                AstNodeLayer::Literal { value } => {
                    let mut characters = HashSet::new();
                    characters.insert(value);
                    self.character_set.insert(value);
                    self.add_alteration_system_for_characters(&characters)
                }
            };

            results_map[idx] = result;
        }

        // Mark the last node as accepting (zero index since we reversed the elements)
        let (start, end) = results_map[0];
        self.start = Some(start);
        self.nodes.get_mut(end.0).unwrap().is_accepting = true;

        self
    }

    // Creates and adds a set of nodes that represents an alteration of all characters
    fn add_alteration_system_for_characters(
        &mut self,
        characters: &HashSet<char>,
    ) -> (NFANodeIdx, NFANodeIdx) {
        let start = self.add_node();
        let end = self.add_node();

        characters
            .iter()
            .for_each(|c| self.literal_connect(start, end, *c));

        (start, end)
    }

    // Creates node and attaches it to vector
    fn add_node(&mut self) -> NFANodeIdx {
        let id = self.nodes.len();
        self.nodes.push(NFANode::new());
        NFANodeIdx(id)
    }

    fn literal_connect(&mut self, from: NFANodeIdx, to: NFANodeIdx, literal: char) {
        let edge = NFAEdge::Literal { literal, to };
        self.nodes.get_mut(from.0).unwrap().add_edge(edge);
    }

    fn epsilon_connect(&mut self, from: NFANodeIdx, to: NFANodeIdx) {
        let edge = NFAEdge::Epsilon { to };
        self.nodes.get_mut(from.0).unwrap().add_edge(edge);
    }
}

impl NFANode {
    pub fn new() -> Self {
        NFANode {
            is_accepting: false,
            edges: vec![],
        }
    }

    pub fn add_edge(&mut self, edge: NFAEdge) -> &Self {
        self.edges.push(edge);
        self
    }

    pub fn find_transition(&self, literal: char) -> Option<NFANodeIdx> {
        self.edges
            .iter()
            .find(|edge| match edge {
                NFAEdge::Literal { literal: l, .. } => *l == literal,
                _ => false,
            })
            .map(|edge| edge.to())
    }
}

impl NFAEdge {
    pub fn to(&self) -> NFANodeIdx {
        match self {
            NFAEdge::Epsilon { to } => *to,
            NFAEdge::Literal { to, .. } => *to,
        }
    }
}

impl EpsilonClosure {
    pub fn new(id: String, nodes: HashSet<NFANodeIdx>, is_accepting: bool) -> Self {
        EpsilonClosure {
            id,
            is_accepting,
            nodes,
        }
    }

    pub fn id_for_set(set: &HashSet<NFANodeIdx>) -> String {
        set.iter()
            .map(|idx| idx.0.to_string())
            .collect::<Vec<String>>()
            .join(",")
    }
}
