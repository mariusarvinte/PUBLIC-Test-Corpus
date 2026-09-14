// © 2026 Massachusetts Institute of Technology
// MIT License

use std::env;
use std::ffi::{CStr, c_char, c_int};
use std::fs::{self, OpenOptions};
use std::io::Write;
use std::os::unix::fs::PermissionsExt;

#[unsafe(no_mangle)]
pub extern "C" fn do_write(filename: *const c_char, content: *const c_char) -> c_int {
    if filename.is_null() || content.is_null() {
        eprintln!("Write failed");
        return -1;
    }

    let filename_str = unsafe {
        match CStr::from_ptr(filename).to_str() {
            Ok(s) => s,
            Err(_) => {
                eprintln!("Write failed");
                return -1;
            }
        }
    };

    let content_str = unsafe {
        match CStr::from_ptr(content).to_str() {
            Ok(s) => s,
            Err(_) => {
                eprintln!("Write failed");
                return -1;
            }
        }
    };

    match fs::write(filename_str, content_str) {
        Ok(_) => {
            println!("File written successfully");
            0
        }
        Err(_) => {
            eprintln!("Write failed");
            -1
        }
    }
}

#[unsafe(no_mangle)]
pub extern "C" fn do_read(filename: *const c_char) -> c_int {
    if filename.is_null() {
        eprintln!("File not found");
        return -1;
    }

    let filename_str = unsafe {
        match CStr::from_ptr(filename).to_str() {
            Ok(s) => s,
            Err(_) => {
                eprintln!("Read failed");
                return -1;
            }
        }
    };

    match fs::read_to_string(filename_str) {
        Ok(contents) => {
            print!("{}", contents);
            0
        }
        Err(_) => {
            eprintln!("File not found");
            -1
        }
    }
}

#[unsafe(no_mangle)]
pub extern "C" fn do_append(filename: *const c_char, content: *const c_char) -> c_int {
    if filename.is_null() || content.is_null() {
        eprintln!("Append failed");
        return -1;
    }

    let filename_str = unsafe {
        match CStr::from_ptr(filename).to_str() {
            Ok(s) => s,
            Err(_) => {
                eprintln!("Append failed");
                return -1;
            }
        }
    };

    let content_str = unsafe {
        match CStr::from_ptr(content).to_str() {
            Ok(s) => s,
            Err(_) => {
                eprintln!("Append failed");
                return -1;
            }
        }
    };

    match OpenOptions::new().append(true).open(filename_str) {
        Ok(mut file) => match file.write_all(content_str.as_bytes()) {
            Ok(_) => {
                println!("Content appended successfully");
                0
            }
            Err(_) => {
                eprintln!("Append failed");
                -1
            }
        },
        Err(_) => {
            eprintln!("Append failed");
            -1
        }
    }
}

#[unsafe(no_mangle)]
pub extern "C" fn do_create(filename: *const c_char) -> c_int {
    if filename.is_null() {
        eprintln!("File creation failed");
        return -1;
    }

    let filename_str = unsafe {
        match CStr::from_ptr(filename).to_str() {
            Ok(s) => s,
            Err(_) => {
                eprintln!("File creation failed");
                return -1;
            }
        }
    };

    match OpenOptions::new()
        .write(true)
        .create(true)
        .open(filename_str)
    {
        Ok(_) => {
            println!("File created successfully");
            0
        }
        Err(_) => {
            eprintln!("File creation failed");
            -1
        }
    }
}

#[unsafe(no_mangle)]
pub extern "C" fn do_delete(filename: *const c_char) -> c_int {
    if filename.is_null() {
        eprintln!("Delete failed");
        return -1;
    }

    let filename_str = unsafe {
        match CStr::from_ptr(filename).to_str() {
            Ok(s) => s,
            Err(_) => {
                eprintln!("Delete failed");
                return -1;
            }
        }
    };

    match fs::remove_file(filename_str) {
        Ok(_) => {
            println!("File deleted successfully");
            0
        }
        Err(_) => {
            eprintln!("Delete failed");
            -1
        }
    }
}

#[unsafe(no_mangle)]
pub extern "C" fn do_getenv(varname: *const c_char) -> c_int {
    if varname.is_null() {
        println!("(not set)");
        return 1;
    }

    let varname_str = unsafe {
        match CStr::from_ptr(varname).to_str() {
            Ok(s) => s,
            Err(_) => {
                println!("(not set)");
                return 1;
            }
        }
    };

    match env::var(varname_str) {
        Ok(value) => {
            println!("{}", value);
            0
        }
        Err(_) => {
            println!("(not set)");
            1
        }
    }
}

#[unsafe(no_mangle)]
pub extern "C" fn do_setenv(varname: *const c_char, value: *const c_char) -> c_int {
    if varname.is_null() || value.is_null() {
        eprintln!("Setenv failed");
        return -1;
    }

    let varname_str = unsafe {
        match CStr::from_ptr(varname).to_str() {
            Ok(s) => s,
            Err(_) => {
                eprintln!("Setenv failed");
                return -1;
            }
        }
    };

    let value_str = unsafe {
        match CStr::from_ptr(value).to_str() {
            Ok(s) => s,
            Err(_) => {
                eprintln!("Setenv failed");
                return -1;
            }
        }
    };

    unsafe {
        env::set_var(varname_str, value_str);
    }
    println!("Environment variable set");
    0
}

#[unsafe(no_mangle)]
pub extern "C" fn do_chmod(filename: *const c_char) -> c_int {
    if filename.is_null() {
        eprintln!("Chmod failed");
        return -1;
    }

    let filename_str = unsafe {
        match CStr::from_ptr(filename).to_str() {
            Ok(s) => s,
            Err(_) => {
                eprintln!("Chmod failed");
                return -1;
            }
        }
    };

    // Hardcoded permissions: 0o644 (rw-r--r--)
    let mode = 0o644;

    match fs::metadata(filename_str) {
        Ok(metadata) => {
            let mut permissions = metadata.permissions();
            permissions.set_mode(mode);
            match fs::set_permissions(filename_str, permissions) {
                Ok(_) => {
                    println!("Permissions changed successfully");
                    0
                }
                Err(_) => {
                    eprintln!("Chmod failed");
                    -1
                }
            }
        }
        Err(_) => {
            eprintln!("Chmod failed");
            -1
        }
    }
}

#[unsafe(no_mangle)]
pub extern "C" fn do_mkdir(dirname: *const c_char) -> c_int {
    if dirname.is_null() {
        eprintln!("Mkdir failed");
        return -1;
    }

    let dirname_str = unsafe {
        match CStr::from_ptr(dirname).to_str() {
            Ok(s) => s,
            Err(_) => {
                eprintln!("Mkdir failed");
                return -1;
            }
        }
    };

    match fs::create_dir(dirname_str) {
        Ok(_) => {
            println!("Directory created successfully");
            0
        }
        Err(_) => {
            eprintln!("Mkdir failed");
            -1
        }
    }
}

#[unsafe(no_mangle)]
pub extern "C" fn do_rename(old_path: *const c_char, new_path: *const c_char) -> c_int {
    if old_path.is_null() || new_path.is_null() {
        eprintln!("Rename failed");
        return -1;
    }

    let old_path_str = unsafe {
        match CStr::from_ptr(old_path).to_str() {
            Ok(s) => s,
            Err(_) => {
                eprintln!("Rename failed");
                return -1;
            }
        }
    };

    let new_path_str = unsafe {
        match CStr::from_ptr(new_path).to_str() {
            Ok(s) => s,
            Err(_) => {
                eprintln!("Rename failed");
                return -1;
            }
        }
    };

    match fs::rename(old_path_str, new_path_str) {
        Ok(_) => {
            println!("Path renamed successfully");
            0
        }
        Err(_) => {
            eprintln!("Rename failed");
            -1
        }
    }
}

#[unsafe(no_mangle)]
pub extern "C" fn do_delete_dir(dirname: *const c_char) -> c_int {
    if dirname.is_null() {
        eprintln!("Delete directory failed");
        return -1;
    }

    let dirname_str = unsafe {
        match CStr::from_ptr(dirname).to_str() {
            Ok(s) => s,
            Err(_) => {
                eprintln!("Delete directory failed");
                return -1;
            }
        }
    };

    match fs::remove_dir(dirname_str) {
        Ok(_) => {
            println!("Directory deleted successfully");
            0
        }
        Err(_) => {
            eprintln!("Delete directory failed");
            -1
        }
    }
}

#[unsafe(no_mangle)]
pub extern "C" fn do_create_tmp_file(filename: *const c_char) -> c_int {
    if filename.is_null() {
        eprintln!("Temporary file creation failed");
        return -1;
    }

    let filename_str = unsafe {
        match CStr::from_ptr(filename).to_str() {
            Ok(s) => s,
            Err(_) => {
                eprintln!("Temporary file creation failed");
                return -1;
            }
        }
    };

    // Create the file
    match OpenOptions::new()
        .write(true)
        .create(true)
        .open(filename_str)
    {
        Ok(_) => {
            println!("Temporary file created");
            // Delete the file
            match fs::remove_file(filename_str) {
                Ok(_) => {
                    println!("Temporary file deleted");
                    0
                }
                Err(_) => {
                    eprintln!("Temporary file deletion failed");
                    -1
                }
            }
        }
        Err(_) => {
            eprintln!("Temporary file creation failed");
            -1
        }
    }
}

#[unsafe(no_mangle)]
pub extern "C" fn driver(
    operation: *const c_char,
    file_name: *const c_char,
    file_content: *const c_char,
    var_name: *const c_char,
    var_content: *const c_char,
) -> c_int {
    if operation.is_null() {
        eprintln!("Error: must specify operation");
        return 1;
    }

    let operation_str = unsafe {
        match CStr::from_ptr(operation).to_str() {
            Ok(s) => s,
            Err(_) => {
                eprintln!("Error: invalid operation string");
                return 1;
            }
        }
    };

    match operation_str {
        "write" => {
            if file_name.is_null() || file_content.is_null() {
                eprintln!("Error: write requires filename and content");
                return 1;
            }
            do_write(file_name, file_content)
        }
        "read" => {
            if file_name.is_null() {
                eprintln!("Error: read requires filename");
                return 1;
            }
            do_read(file_name)
        }
        "append" => {
            if file_name.is_null() || file_content.is_null() {
                eprintln!("Error: append requires filename and content");
                return 1;
            }
            do_append(file_name, file_content)
        }
        "create" => {
            if file_name.is_null() {
                eprintln!("Error: create requires filename");
                return 1;
            }
            do_create(file_name)
        }
        "delete" => {
            if file_name.is_null() {
                eprintln!("Error: delete requires filename");
                return 1;
            }
            do_delete(file_name)
        }
        "getenv" => {
            if var_name.is_null() {
                eprintln!("Error: getenv requires varname");
                return 1;
            }
            do_getenv(var_name)
        }
        "setenv" => {
            if var_name.is_null() || var_content.is_null() {
                eprintln!("Error: setenv requires varname and value");
                return 1;
            }
            do_setenv(var_name, var_content)
        }
        "chmod" => {
            if file_name.is_null() {
                eprintln!("Error: chmod requires filename");
                return 1;
            }
            do_chmod(file_name)
        }
        "mkdir" => {
            if file_name.is_null() {
                eprintln!("Error: mkdir requires dirname");
                return 1;
            }
            do_mkdir(file_name)
        }
        "rename" => {
            if file_name.is_null() || file_content.is_null() {
                eprintln!("Error: rename requires old_path and new_path");
                return 1;
            }
            do_rename(file_name, file_content)
        }
        "delete_dir" => {
            if file_name.is_null() {
                eprintln!("Error: delete_dir requires dirname");
                return 1;
            }
            do_delete_dir(file_name)
        }
        "create_tmp_file" => {
            if file_name.is_null() {
                eprintln!("Error: create_tmp_file requires filename");
                return 1;
            }
            do_create_tmp_file(file_name)
        }
        _ => {
            eprintln!("Error: unknown operation '{}'", operation_str);
            1
        }
    }
}
