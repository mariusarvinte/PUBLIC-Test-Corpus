// © 2026 Massachusetts Institute of Technology
// MIT License

use std::{
    ffi::c_double,
    io::{self, BufRead, Write},
    time::Duration,
};

pub fn read_write() {
    let mut input = String::new();

    // Prompt the user for input
    print!("Please enter some text: ");
    io::stdout().flush().unwrap(); // Ensure the prompt is displayed immediately

    // Read input from stdin
    let stdin = io::stdin();
    let mut handle = stdin.lock();

    match handle.read_line(&mut input) {
        Ok(_) => {
            // Remove the trailing newline character, if present
            let trimmed = input.trim_end();

            // Write the sanitized input to stdout
            println!("You entered: {}", trimmed);
        }
        Err(e) => {
            // Handle input error
            eprintln!("Error reading input: {}", e);
        }
    }

    eprintln!("Printing to stderr");
}

/// Purposfully segfault by dereferncing nullptr
pub fn segfault() {
    std::thread::sleep(Duration::from_secs(2));
    unsafe {
        let ptr: *mut i32 = 0x1 as *mut i32;
        *ptr = 42;
    }
}

pub fn timeout() {
    std::thread::sleep(Duration::from_secs(10));
    println!("This should never be printed");
}

#[unsafe(no_mangle)]
pub extern "C" fn conduct_me(a: i32, b: i32) -> i32 {
    println!("INSIDE RUST");
    if a < 0 {
        read_write();
    } else if a == 6 {
        let value = std::env::var("CANDO_TEST_VECTOR_ENV").unwrap_or_else(|_| "<unset>".into());
        println!("{}", value);
    } else if a == 42 {
        segfault();
    } else if b == 42 {
        panic!("I panicked");
    } else if a == 2 {
        timeout();
    } else {
        println!("Hello World");
    }

    a + b
}

#[unsafe(no_mangle)]
pub extern "C" fn add_float(a: c_double, b: c_double) -> c_double {
    a + b
}
