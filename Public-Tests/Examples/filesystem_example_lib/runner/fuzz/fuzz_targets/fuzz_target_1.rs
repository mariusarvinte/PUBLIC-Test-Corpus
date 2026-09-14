// © 2026 Massachusetts Institute of Technology
// MIT License

#![no_main]

use libfuzzer_sys::fuzz_target;
use std::ffi::CString;
use std::os::raw::c_char;

#[link(name = "fileops")]
extern "C" {
    fn write_file(filename: *const c_char, content: *const c_char) -> i32;
    fn read_file(filename: *const c_char, buffer: *mut *mut c_char) -> i32;
    fn file_exists(filename: *const c_char) -> i32;
    fn set_env(varname: *const c_char, value: *const c_char) -> i32;
}

fuzz_target!(|data: &[u8]| {
    // Split input data into filename and content
    if data.len() < 2 {
        return;
    }

    let split_point = data.len() / 2;
    let filename_data = &data[..split_point];
    let content_data = &data[split_point..];

    // Try to create valid C strings, skip if they contain null bytes
    if let Ok(filename) = CString::new(filename_data) {
        if let Ok(content) = CString::new(content_data) {
            unsafe {
                // Test write operation
                let _ = write_file(filename.as_ptr(), content.as_ptr());

                // Test file_exists
                let _ = file_exists(filename.as_ptr());
            }
        }
    }

    // Test environment variable operations with random data
    if data.len() >= 4 {
        let varname_data = &data[..data.len() / 2];
        let value_data = &data[data.len() / 2..];

        if let Ok(varname) = CString::new(varname_data) {
            if let Ok(value) = CString::new(value_data) {
                unsafe {
                    let _ = set_env(varname.as_ptr(), value.as_ptr());
                }
            }
        }
    }
});
