// © 2026 Massachusetts Institute of Technology
// MIT License

procspawn::enable_test_support!();

mod util;
use cando2::*;

harness! {
    state: {
        input: i32,
        returns: i32
    },

    signature: extern "C" fn(i32) -> i32,

    fn run(&mut self) {
        self.returns = (*SYMBOL)(self.input);
    }
}

#[test]
fn statics() {
    util::init();

    assert_eq!("mock_candidate_lib", *CANDIDATE_NAME);

    procspawn::Builder::new()
        .spawn(
            (),
            |()| {
                RUN_RUST.get_or_init(|| false);
                TEST_ROOT_DIR.get_or_init(|| None);
                let _ = &*SYMBOL;
            },
            None,
        )
        .join()
        .expect("Coudln't initialize SYMBOL");
}

#[test]
fn run() {
    util::init();

    let state = State::zeroed().unwrap();
    assert_eq!(state.input, 0);
    assert_eq!(state.returns, 0);

    let state_out = procspawn::Builder::new()
        .spawn(
            state,
            |mut state| {
                RUN_RUST.get_or_init(|| false);
                TEST_ROOT_DIR.get_or_init(|| None);
                state.run();
                state
            },
            None,
        )
        .join()
        .expect("Couldn't run test case");

    assert_eq!(state_out.input, 0);
    assert_eq!(state_out.returns, 3);
}
