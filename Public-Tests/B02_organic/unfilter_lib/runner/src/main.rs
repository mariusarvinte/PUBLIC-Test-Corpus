// © 2026 Massachusetts Institute of Technology
// MIT License

#![cfg_attr(fuzzing, no_main)]

use cando2::{utils::util, *};

harness! {
    state: {
        w: c_int,
        h: c_int,
        bpp: c_int,
        raw: Vec<u8>,
    },

    signature: unsafe extern "C" fn(c_int, c_int, c_int, *mut u8),

    fn run(&mut self) {
        unsafe {
            (*SYMBOL)(
                self.w,
                self.h,
                self.bpp,
                util::vec_as_mut_ptr(&mut self.raw),
            );
        };
    }
}
