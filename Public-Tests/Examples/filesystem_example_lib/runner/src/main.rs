// © 2026 Massachusetts Institute of Technology
// MIT License

#![cfg_attr(fuzzing, no_main)]

use cando2::{utils::util, *};

harness! {
    state: {
        operation: Option<CString>,
        file_name: Option<CString>,
        file_content: Option<CString>,
        var_name: Option<CString>,
        var_content: Option<CString>,
        rc: c_int,
    },
    library: "fileops",
    symbol: "driver",
    signature: unsafe extern "C" fn(*const c_char, *const c_char, *const c_char, *const c_char, *const c_char) -> c_int,

    fn run(&mut self) {
        self.rc = unsafe {
            (*SYMBOL)(
                util::cstr_as_ptr(&self.operation),
                util::cstr_as_ptr(&self.file_name),
                util::cstr_as_ptr(&self.file_content),
                util::cstr_as_ptr(&self.var_name),
                util::cstr_as_ptr(&self.var_content),
            )
        };
    }
}
