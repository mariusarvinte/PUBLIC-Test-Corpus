// © 2026 Massachusetts Institute of Technology
// MIT License

#![allow(unused_imports)]

procspawn::enable_test_support!();

mod util;
use util::error_as_rc;

use cando2::*;

harness! {
    state: {
        a: i32,
        b: i32,
        returns: i32
    },

    library: "mock_candidate_lib",

    symbol: "wrong_symbol",

    signature: extern "C" fn(i32, i32) -> i32,

    fn run(&mut self) {
        self.returns = (*SYMBOL)(self.a, self.b);
    }
}

// NOTE: These tests can't be run because it forcefully exits with status code 3 which is exactly the
// behavior that we want, but trigger the Rust test runtime to detect and issue. To verify this is
// working you can run the test with `--no-capture` and ensure that it prints something about an
// incorrect symbol name and exits with code 3.

// #[test]
// fn bad_symbol_name_c() {
//     util::init();
//
//     let args = &["--", "-l", "verbose", "-v", "lib_pass.json", "lib"];
//     let ret = conduct_with(args);
//     assert_eq!(error_as_rc(&ret), 3);
// }
//
// #[test]
// fn bad_symbol_name_rust() {
//     util::init();
//
//     let args = &[
//         "--",
//         "-l",
//         "verbose",
//         "-v",
//         "lib_pass.json",
//         "--rust",
//         "lib",
//     ];
//     let ret = conduct_with(args);
//     assert_eq!(error_as_rc(&ret), 3);
// }
