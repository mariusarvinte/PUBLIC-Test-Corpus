// © 2026 Massachusetts Institute of Technology
// MIT License

procspawn::enable_test_support!();

mod util;
use util::*;

use cando2::*;
use std::fs;

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
        self.returns = (*SYMBOL)(self.a, self.b);
    }
}

#[test]
fn test_lib_pass() {
    util::init();

    let vector = "lib_pass.json";
    let args = &["--", "-l", "verbose", "-v", vector, "lib"];
    let ret = conduct_with(args);
    assert_eq!(error_as_rc(&ret), 0);
    assert_result(ret, Some(vector), GenericRunResult::Pass);
}

#[test]
fn test_lib_fail() {
    util::init();

    let vector = "lib_fail.json";
    let args = &["--", "-l", "normal", "-v", vector, "lib"];
    let ret = conduct_with(args);
    assert_eq!(error_as_rc(&ret), 1);
    assert_result(ret, Some(vector), GenericRunResult::VectorComparisonFailed);
}

#[test]
fn test_lib_zero() {
    util::init();

    let args = &["--", "-l", "verbose", "lib", "-z"];
    let ret = conduct_with(args);
    assert_eq!(error_as_rc(&ret), 0);
    assert_result(ret, None, GenericRunResult::NoCompare);
}

// NOTE: For all of the stdout/err tests we have to use regex because `procspawn::init()` isn't
// called until after some of the `cargo test` stuff has run and has already printed output which
// gets captured, so we use a very permissive regex to look for only specific strings at the end

#[test]
fn test_lib_stdout_good() {
    util::init();

    let vector = "lib_stdout_good.json";
    let args = &["--", "-l", "verbose", "-v", vector, "lib"];
    let ret = conduct_with(args);
    assert_eq!(error_as_rc(&ret), 0);
    assert_result(ret, Some(vector), GenericRunResult::Pass);
}

#[test]
fn test_lib_env() {
    util::init();

    let vector = "lib_env.json";
    let args = &["--", "-l", "verbose", "-v", vector, "lib"];
    let ret = conduct_with(args);
    assert_eq!(error_as_rc(&ret), 0);
    assert_result(ret, Some(vector), GenericRunResult::Pass);
}

#[test]
fn test_lib_stdout_bad() {
    util::init();

    let vector = "lib_stdout_bad.json";
    let args = &["--", "-l", "verbose", "-v", vector, "lib"];
    let ret = conduct_with(args);
    assert_eq!(error_as_rc(&ret), 1);
    assert_result(ret, Some(vector), GenericRunResult::VectorComparisonFailed);
}

#[test]
fn test_lib_stdout_none() {
    util::init();

    let vector = "lib_stdout_none.json";
    let args = &["--", "-l", "verbose", "-v", vector, "lib"];
    let ret = conduct_with(args);
    assert_eq!(error_as_rc(&ret), 1);
    assert_result(ret, Some(vector), GenericRunResult::VectorComparisonFailed);
}

#[test]
fn test_lib_stdin_stderr_good() {
    util::init();

    let vector = "lib_stdin_stderr_good.json";
    let args = &["--", "-l", "verbose", "-v", vector, "lib"];
    let ret = conduct_with(args);
    assert_eq!(error_as_rc(&ret), 0);
    assert_result(ret, Some(vector), GenericRunResult::Pass);
}

#[test]
fn test_lib_timeout() {
    util::init();

    let vector = "lib_timeout.json";
    let args = &["--", "-l", "verbose", "-v", vector, "--timeout", "1", "lib"];
    let ret = conduct_with(args);
    assert_eq!(error_as_rc(&ret), 1);
    assert_result(ret, Some(vector), GenericRunResult::Timeout);
}

#[test]
fn test_lib_segfault() {
    util::init();

    let vector = "lib_segfault.json";
    let args = &["--", "-l", "verbose", "-v", vector, "lib"];
    let ret = conduct_with(args);
    assert_eq!(error_as_rc(&ret), 1);
    assert_result(ret, Some(vector), GenericRunResult::SegmentationFault);
}

#[test]
fn test_lib_timeout_segfault() {
    util::init();

    let vector = "lib_segfault.json";
    let args = &["--", "-l", "verbose", "-v", vector, "--timeout", "1", "lib"];
    let ret = conduct_with(args);
    assert_eq!(error_as_rc(&ret), 1);
    assert_result(ret, Some(vector), GenericRunResult::Timeout);
}

#[test]
fn test_lib_output() {
    let vector = "lib_pass.json";
    let fname = "output.json";
    let args = &["--", "-l", "verbose", "-v", vector, "-o", fname, "lib"];
    let ret = conduct_with(args);
    assert_eq!(error_as_rc(&ret), 0);
    assert_result(ret, Some(vector), GenericRunResult::Pass);

    let path = Path::new(fname);
    assert!(path.exists());
    let _ = fs::remove_file(path);
}
