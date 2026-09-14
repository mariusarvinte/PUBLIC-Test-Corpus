// © 2026 Massachusetts Institute of Technology
// MIT License

procspawn::enable_test_support!();

mod util;
use cando2::runners::bin_runner::conduct_bin;
use util::error_as_rc;

#[test]
fn bad_bin_name_c() {
    util::init();

    let args = &[
        "--",
        "-l",
        "verbose",
        "-v",
        "bin_good.json",
        "bin",
        "--name",
        "bad",
    ];
    let ret = conduct_bin(args);
    assert_eq!(error_as_rc(&ret), 2);
}

#[test]
fn bad_bin_name_rust() {
    util::init();

    let args = &[
        "--",
        "-l",
        "verbose",
        "-v",
        "bin_good.json",
        "--rust",
        "bin",
        "--name",
        "bad",
    ];
    let ret = conduct_bin(args);
    assert_eq!(error_as_rc(&ret), 2);
}
