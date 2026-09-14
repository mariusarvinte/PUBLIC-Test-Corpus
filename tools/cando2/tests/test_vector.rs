// © 2026 Massachusetts Institute of Technology
// MIT License

procspawn::enable_test_support!();

mod util;

use cando2::*;

harness! {
    state: {
        a: i32,
        b: i32,
        returns: i32
    },

    signature: extern "C" fn(),

    fn run(&mut self) {}
}

#[test]
fn load_from_json() {
    util::init();

    let json = r#"
    {
        "lib_state_in": {
            "a": 1,
            "b": 2,
            "returns": 0
        },
        "lib_state_out": {
            "a": 1,
            "b": 2,
            "returns": 0
        }
    }
    "#;
    let tc = TestVector::<State, ExpectedState>::from_json(json).unwrap();
    assert_eq!(tc.lib_state_in.unwrap(), tc.lib_state_out.unwrap());
}

#[test]
fn eq_with_skipped_field() {
    util::init();

    let json = r#"
    {
        "lib_state_in": {
            "a": 1,
            "b": 2,
            "returns": 0
        },
        "lib_state_out": {
            "a": 1,
            "b": 2
        }
    }
    "#;
    let tc = TestVector::<State, ExpectedState>::from_json(json).unwrap();
    assert_eq!(tc.lib_state_in.unwrap(), tc.lib_state_out.unwrap());
}

#[test]
fn load_from_file() {
    util::init();

    let path = "tests/test_data/mock_candidate_lib/test_vectors/lib_pass.json";
    let tc = TestVector::<State, ExpectedState>::from_file(path)
        .unwrap()
        .unwrap();
    assert_eq!(tc.lib_state_in.unwrap().a, 1);
    assert_eq!(tc.lib_state_out.unwrap().returns.unwrap(), 3);
}

#[test]
fn load_from_file_emtpy() {
    util::init();

    let path = "tests/test_data/mock_candidate_lib/test_vectors/empty.json";
    let tc = TestVector::<State, ExpectedState>::from_file(path).unwrap();
    assert!(tc.is_none());
}

#[test]
fn load_from_corpus() {
    util::init();

    let tc = TestVector::<State, ExpectedState>::from_test_vector_dir(
        &CONTEXT.test_vector_dir,
        Path::new("lib_pass.json"),
    )
    .unwrap()
    .unwrap();
    assert_eq!(tc.lib_state_in.unwrap().a, 1);
    assert_eq!(tc.lib_state_out.unwrap().returns.unwrap(), 3);
}

#[test]
fn load_entire_corpus() {
    util::init();

    let tcs = TestVector::<State, ExpectedState>::load_all(&CONTEXT.test_vector_dir).unwrap();
    let tc1 = &tcs["lib_pass.json"];
    assert_eq!(tc1.lib_state_in.as_ref().unwrap().a, 1);
    assert_eq!(tc1.lib_state_out.as_ref().unwrap().returns.unwrap(), 3);
    let tc2 = tcs["bin_pass.json"].clone();
    assert_eq!(tc2.stdout.unwrap().pattern, "Hello, world!\n");
}

#[test]
fn load_lib_state() {
    util::init();

    let tc = TestVector::<State, ExpectedState>::from_test_vector_dir(
        &CONTEXT.test_vector_dir,
        Path::new("lib_pass.json"),
    )
    .unwrap()
    .unwrap();
    let state = tc.lib_state().unwrap();
    assert_eq!(state.a, 1);
    assert_eq!(state.b, 2);
    assert_eq!(state.returns, 0);
}
