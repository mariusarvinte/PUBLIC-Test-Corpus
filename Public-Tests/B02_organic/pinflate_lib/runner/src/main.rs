// © 2026 Massachusetts Institute of Technology
// MIT License

#![cfg_attr(fuzzing, no_main)]

use cando2::{utils::util, *};

harness! {
    state: {
        in_bytes: c_int,
        out_bytes: c_int,
        b_in: Vec<u8>,
        b_out: Vec<u8>,
        returns: c_int
    },

    signature: unsafe extern "C" fn(*mut u8, c_int, *mut u8, c_int) -> c_int,

    fn run(&mut self) {
        self.b_out.resize(self.out_bytes as usize, 0);
        unsafe {
            self.returns = (*SYMBOL)(
                util::vec_as_mut_ptr(&mut self.b_in),
                self.in_bytes,
                util::vec_as_mut_ptr(&mut self.b_out),
                self.out_bytes
            );
        };
    }
}
