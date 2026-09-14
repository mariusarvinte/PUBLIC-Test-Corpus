// © 2026 Massachusetts Institute of Technology
// MIT License

procspawn::enable_test_support!();

use cando2::runners::bin_runner::conduct_bin;

mod util;
use util::*;

#[test]
fn test_bin_pass() {
    util::init();

    let vector = "bin_pass.json";
    let args = &["--", "-l", "verbose", "-v", vector, "bin"];
    let ret = conduct_bin(args);
    assert_eq!(error_as_rc(&ret), 0);
    assert_result(ret, Some(vector), GenericRunResult::Pass);
}

#[test]
fn test_bin_fail() {
    util::init();

    let vector = "bin_fail.json";
    let args = &["--", "-l", "verbose", "-v", vector, "bin"];
    let ret = conduct_bin(args);
    assert_eq!(error_as_rc(&ret), 1);
    assert_result(ret, Some(vector), GenericRunResult::VectorComparisonFailed);
}

#[test]
fn test_bin_stdout_none() {
    util::init();

    let vector = "bin_stdout_none.json";
    let args = &["--", "-l", "verbose", "-v", vector, "bin"];
    let ret = conduct_bin(args);
    assert_eq!(error_as_rc(&ret), 1);
    assert_result(ret, Some(vector), GenericRunResult::VectorComparisonFailed);
}

#[test]
fn test_bin_read_write() {
    util::init();

    let vector = "bin_read_write.json";
    let args = &["--", "-l", "verbose", "-v", vector, "bin"];
    let ret = conduct_bin(args);
    assert_eq!(error_as_rc(&ret), 0);
    assert_result(ret, Some(vector), GenericRunResult::Pass);
}

#[test]
fn test_bin_env() {
    util::init();

    let vector = "bin_env.json";
    let args = &["--", "-l", "verbose", "-v", vector, "bin"];
    let ret = conduct_bin(args);
    assert_eq!(error_as_rc(&ret), 0);
    assert_result(ret, Some(vector), GenericRunResult::Pass);
}

#[test]
fn test_bin_segfault() {
    util::init();

    let vector = "bin_segfault.json";
    let args = &["--", "-l", "verbose", "-v", vector, "bin"];
    let ret = conduct_bin(args);
    assert_eq!(error_as_rc(&ret), 1);
    assert_result(ret, Some(vector), GenericRunResult::SegmentationFault);
}

#[test]
fn test_bin_rc() {
    util::init();

    let vector = "bin_rc.json";
    let args = &["--", "-l", "verbose", "-v", vector, "bin"];
    let ret = conduct_bin(args);
    assert_eq!(error_as_rc(&ret), 0);
    assert_result(ret, Some(vector), GenericRunResult::Pass);
}

#[test]
fn test_bin_timeout() {
    util::init();

    let vector = "bin_timeout.json";
    let args = &["--", "-l", "verbose", "-v", vector, "--timeout", "1", "bin"];
    let ret = conduct_bin(args);
    assert_eq!(error_as_rc(&ret), 1);
    assert_result(ret, Some(vector), GenericRunResult::Timeout);
}
