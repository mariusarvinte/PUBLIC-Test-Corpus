// © 2026 Massachusetts Institute of Technology
// MIT License

procspawn::enable_test_support!();

mod util;
use cando2::*;
use util::*;

harness! {
    state: {
        a: c_double,
        b: c_double,
        returns: c_double
    },

    library: "mock_candidate_lib",

    symbol: "add_float",

    signature: extern "C" fn(c_double, c_double) -> c_double,

    fn run(&mut self) {
        self.returns = (*SYMBOL)(self.a, self.b);
    }
}

#[test]
fn test_double_pass_c() {
    util::init();

    let vector = "double/pass.json";
    let args = &["--", "-l", "verbose", "-v", vector, "lib"];
    let ret = conduct_with(args);
    assert_eq!(error_as_rc(&ret), 0);
    assert_result(ret, Some(vector), GenericRunResult::Pass);
}

#[test]
fn test_double_fail_c() {
    util::init();

    let vector = "double/fail.json";
    let args = &["--", "-l", "verbose", "-v", vector, "lib"];
    let ret = conduct_with(args);
    assert_eq!(error_as_rc(&ret), 1);
    assert_result(ret, Some(vector), GenericRunResult::VectorComparisonFailed);
}

#[test]
fn test_double_pass_rust() {
    util::init();

    let vector = "double/pass.json";
    let args = &["--", "-l", "verbose", "-v", vector, "--rust", "lib"];
    let ret = conduct_with(args);
    assert_eq!(error_as_rc(&ret), 0);
    assert_result(ret, Some(vector), GenericRunResult::Pass);
}

#[test]
fn test_double_fail_rust() {
    util::init();

    let vector = "double/fail.json";
    let args = &["--", "-l", "verbose", "-v", vector, "--rust", "lib"];
    let ret = conduct_with(args);
    assert_eq!(error_as_rc(&ret), 1);
    assert_result(ret, Some(vector), GenericRunResult::VectorComparisonFailed);
}
