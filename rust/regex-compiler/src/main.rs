use std::{
    env,
    io::{self, Write},
};

use regex_compiler::Regex;

fn main() {
    let args: Vec<String> = env::args().collect();
    if args.len() != 2 {
        eprintln!("Usage: {} <pattern>", args[0]);
        std::process::exit(1);
    }

    let pattern = &args[1];
    let regex = Regex::new(pattern);

    let mut buffer = String::new();
    loop {
        println!("Pattern: {}", pattern);
        print!("  Input: ");
        io::stdout().flush().expect("Failed to flush stdout");

        buffer.clear();
        io::stdin()
            .read_line(&mut buffer)
            .expect("Failed to read line");

        if buffer.trim().is_empty() {
            break;
        }

        let result_str = match regex.accepts(buffer.trim()) {
            true => "ACCEPTED",
            false => "REJECTED",
        };
        println!("  Result: {}", result_str);
    }
}
