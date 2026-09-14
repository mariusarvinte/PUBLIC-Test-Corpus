// © 2026 Massachusetts Institute of Technology
// MIT License

#![cfg_attr(fuzzing, no_main)]

use cando2::{utils::util, *};

state_member! {
    struct cp_pixel_t {
        r: u8,
        g: u8,
        b: u8,
        a: u8
    }
}

harness! {
    state: {
        bpp: c_int,
        w: c_int,
        h: c_int,
        src: Vec<u8>,
        dst: Vec<cp_pixel_t>,
    },

    signature: unsafe extern "C" fn(c_int, c_int, c_int, *const u8, *mut cp_pixel_t),

    fn run(&mut self) {
        self.dst.resize((self.w * self.h) as usize, cp_pixel_t { r: 0, g: 0, b: 0, a: 0 });
        unsafe {
            (*SYMBOL)(
                self.bpp,
                self.w,
                self.h,
                util::vec_as_ptr(&self.src),
                util::vec_as_mut_ptr(&mut self.dst),
            );
        };
    }
}
