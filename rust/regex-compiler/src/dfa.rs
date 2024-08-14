use std::collections::HashMap;

use crate::nfa::{EpsilonClosure, NFA};

pub struct DFA {
    nodes: HashMap<DFANodeId, DFANode>,
    start: DFANodeId,
}

struct DFANode {
    id: DFANodeId,
    is_accepting: bool,
    edges: Vec<DFAEdge>,
}

#[derive(Clone, PartialEq, Eq, Hash)]
struct DFANodeId(String);

struct DFAEdge {
    literal: char,
    to: DFANodeId,
}

impl DFA {
    pub fn from_nfa(nfa: &NFA) -> Self {
        // Map of dfa nodes that we build up
        let mut dfa_nodes_map: HashMap<DFANodeId, DFANode> = HashMap::new();
        // Stack of eclosures to process
        let mut eclosures_stack = Vec::new();

        // Create initial eclosure from starting node of nfa, then create dfa_node from the eclosure
        let initial_closure = nfa.epsilon_closure(nfa.start());
        // Create the initial dfa node from the initial closure
        let initial_dfa_node = DFANode::from_eclosure(&initial_closure);
        let initial_dfa_node_id = initial_dfa_node.id.clone();

        eclosures_stack.push(initial_closure);
        dfa_nodes_map.insert(initial_dfa_node_id.clone(), initial_dfa_node);

        // Process the eclosures stack
        while let Some(eclosure) = eclosures_stack.pop() {
            for literal in nfa.character_set_iter() {
                // Get the move set of the eclosure for the current literal
                let move_set = nfa.compute_move_set(&eclosure, *literal);
                if (move_set).is_empty() {
                    continue;
                }
                let next_dfa_node_id = DFANodeId(EpsilonClosure::id_for_set(&move_set));

                match dfa_nodes_map.get(&next_dfa_node_id) {
                    Some(_) => (),
                    None => {
                        let next_closure = nfa.epsilon_closure_set(move_set);
                        let next_dfa_node = DFANode::from_eclosure(&next_closure);
                        dfa_nodes_map.insert(next_dfa_node_id.clone(), next_dfa_node);
                        eclosures_stack.push(next_closure);
                    }
                };

                // Add an edge from current_dfa_node to the next_dfa_node
                let current_dfa_node = dfa_nodes_map
                    .get_mut(&DFANodeId(eclosure.id.clone()))
                    .unwrap();
                current_dfa_node.add_edge(*literal, next_dfa_node_id);
            }
        }

        DFA {
            nodes: dfa_nodes_map,
            start: initial_dfa_node_id,
        }
    }

    pub fn accepts(&self, input: &str) -> bool {
        let mut current_node = &self.nodes[&self.start];

        for c in input.chars() {
            let next_node = match current_node.edges.iter().find(|edge| edge.literal == c) {
                Some(edge) => &self.nodes[&edge.to],
                None => return false,
            };

            current_node = next_node;
        }

        current_node.is_accepting
    }
}

impl DFANode {
    pub fn from_eclosure(eclosure: &EpsilonClosure) -> Self {
        let edges = Vec::new();

        DFANode {
            id: DFANodeId(eclosure.id.clone()),
            is_accepting: eclosure.is_accepting,
            edges,
        }
    }

    pub fn add_edge(&mut self, literal: char, to: DFANodeId) {
        self.edges.push(DFAEdge { literal, to });
    }
}
