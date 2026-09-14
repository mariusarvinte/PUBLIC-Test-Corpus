// © 2026 Massachusetts Institute of Technology
// MIT License

procspawn::enable_test_support!();

mod util;
use cando2::*;
use util::*;

harness! {
    state: {
        a: i32,
        b: i32,
        returns: i32
    },

    library: "mock_candidate_lib",

    symbol: "conduct_me",

    signature: extern "C" fn(i32, i32) -> i32,

    fn run(&mut self) {
        panic!("Couldn't run the run function");
    }
}

#[test]
fn run_panic() {
    util::init();

    let vector = "lib_pass.json";
    let args = &["--", "-l", "verbose", "-v", vector, "lib"];
    let ret = conduct_with(args);
    assert_eq!(error_as_rc(&ret), 5);
}
