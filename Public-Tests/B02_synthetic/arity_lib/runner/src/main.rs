// © 2026 Massachusetts Institute of Technology
// MIT License

#![cfg_attr(fuzzing, no_main)]

use cando2::*;

harness! {
    state: {
        len: u8,
        params: [i32; 4],
        returns: i32
    },

    signature: unsafe extern "C" fn(u8, *const i32) -> i32,

    fn run(&mut self) {
        self.returns = unsafe {
            (*SYMBOL)(
                self.len,
                &raw const self.params as *const i32
            )
        }
    }
}
