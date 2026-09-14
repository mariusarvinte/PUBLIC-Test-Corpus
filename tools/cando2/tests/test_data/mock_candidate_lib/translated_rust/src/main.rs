// © 2026 Massachusetts Institute of Technology
// MIT License

use mock_candidate_lib::{read_write, segfault, timeout};

fn main() {
    let args: Vec<String> = std::env::args().collect();

    if args.len() == 1 {
        println!("Hello, world!");
    } else if args[1] == "1" {
        read_write();
    } else if args[1] == "2" {
        panic!("Uh oh");
    } else if args[1] == "3" {
        segfault();
    } else if args[1] == "4" {
        std::process::exit(1);
    } else if args[1] == "5" {
        timeout();
    } else if args[1] == "6" {
        let value = std::env::var("CANDO_TEST_VECTOR_ENV").unwrap_or_else(|_| "<unset>".into());
        println!("{}", value);
    }
}
