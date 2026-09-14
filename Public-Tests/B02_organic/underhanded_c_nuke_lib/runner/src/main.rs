// © 2026 Massachusetts Institute of Technology
// MIT License

#![cfg_attr(fuzzing, no_main)]

use cando2::{utils::util, *};

harness! {
    state: {
        test: Vec<c_double>,
        reference: Vec<c_double>,
        bins: c_int,
        threshold: c_double,
        returns: c_int
    },

    library: "underhanded_c_nuke_lib",
    symbol: "match",
    signature: unsafe extern "C" fn(*const c_double, *const c_double, c_int, c_double) -> c_int,

    fn run(&mut self) {
        self.returns = unsafe {
            (*SYMBOL)(
                util::vec_as_ptr(&self.test),
                util::vec_as_ptr(&self.reference),
                self.bins,
                self.threshold,
            )
        };
    }
}
