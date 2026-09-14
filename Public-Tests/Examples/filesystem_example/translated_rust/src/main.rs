// © 2026 Massachusetts Institute of Technology
// MIT License

use std::env;
use std::fs::{self, OpenOptions};
use std::io::Write;
use std::os::unix::fs::PermissionsExt;
use std::process;

fn do_write(filename: &str, content: &str) -> i32 {
    match fs::write(filename, content) {
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

fn do_read(filename: &str) -> i32 {
    match fs::read_to_string(filename) {
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

fn do_append(filename: &str, content: &str) -> i32 {
    match OpenOptions::new().append(true).open(filename) {
        Ok(mut file) => match file.write_all(content.as_bytes()) {
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

fn do_create(filename: &str) -> i32 {
    match OpenOptions::new().write(true).create(true).open(filename) {
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

fn do_delete(filename: &str) -> i32 {
    match fs::remove_file(filename) {
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

fn do_getenv(varname: &str) -> i32 {
    match env::var(varname) {
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

fn do_setenv(varname: &str, value: &str) -> i32 {
    unsafe {
        env::set_var(varname, value);
    }
    println!("Environment variable set");
    0
}

fn do_chmod(filename: &str) -> i32 {
    // Hardcoded permissions: 0o644 (rw-r--r--)
    let mode = 0o644;

    match fs::metadata(filename) {
        Ok(metadata) => {
            let mut permissions = metadata.permissions();
            permissions.set_mode(mode);
            match fs::set_permissions(filename, permissions) {
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

fn do_mkdir(dirname: &str) -> i32 {
    match fs::create_dir(dirname) {
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

fn do_rename(old_path: &str, new_path: &str) -> i32 {
    match fs::rename(old_path, new_path) {
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

fn do_delete_dir(dirname: &str) -> i32 {
    match fs::remove_dir(dirname) {
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

fn do_create_tmp_file(filename: &str) -> i32 {
    // Create the file
    match OpenOptions::new().write(true).create(true).open(filename) {
        Ok(_) => {
            println!("Temporary file created");
            // Delete the file
            match fs::remove_file(filename) {
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

fn print_usage(progname: &str) {
    eprintln!("Usage: {} <operation> <args...>", progname);
    eprintln!("Operations:");
    eprintln!("  write <filename> <content>");
    eprintln!("  read <filename>");
    eprintln!("  append <filename> <content>");
    eprintln!("  create <filename> <content>");
    eprintln!("  delete <filename>");
    eprintln!("  getenv <varname>");
    eprintln!("  setenv <varname> <value>");
    eprintln!("  chmod <filename>");
    eprintln!("  mkdir <dirname>");
    eprintln!("  rename <old_path> <new_path>");
    eprintln!("  delete_dir <dirname>");
    eprintln!("  create_tmp_file <filename>");
}

fn main() {
    let args: Vec<String> = env::args().collect();

    if args.len() < 2 {
        print_usage(&args[0]);
        process::exit(1);
    }

    let operation = &args[1];

    let exit_code = match operation.as_str() {
        "write" => {
            if args.len() != 4 {
                eprintln!("Error: write requires filename and content");
                1
            } else {
                do_write(&args[2], &args[3])
            }
        }
        "read" => {
            if args.len() != 3 {
                eprintln!("Error: read requires filename");
                1
            } else {
                do_read(&args[2])
            }
        }
        "append" => {
            if args.len() != 4 {
                eprintln!("Error: append requires filename and content");
                1
            } else {
                do_append(&args[2], &args[3])
            }
        }
        "create" => {
            if args.len() != 3 {
                eprintln!("Error: create requires filename");
                1
            } else {
                do_create(&args[2])
            }
        }
        "delete" => {
            if args.len() != 3 {
                eprintln!("Error: delete requires filename");
                1
            } else {
                do_delete(&args[2])
            }
        }
        "getenv" => {
            if args.len() != 3 {
                eprintln!("Error: getenv requires varname");
                1
            } else {
                do_getenv(&args[2])
            }
        }
        "setenv" => {
            if args.len() != 4 {
                eprintln!("Error: setenv requires varname and value");
                1
            } else {
                do_setenv(&args[2], &args[3])
            }
        }
        "chmod" => {
            if args.len() != 3 {
                eprintln!("Error: chmod requires filename");
                1
            } else {
                do_chmod(&args[2])
            }
        }
        "mkdir" => {
            if args.len() != 3 {
                eprintln!("Error: mkdir requires dirname");
                1
            } else {
                do_mkdir(&args[2])
            }
        }
        "rename" => {
            if args.len() != 4 {
                eprintln!("Error: rename requires old_path and new_path");
                1
            } else {
                do_rename(&args[2], &args[3])
            }
        }
        "delete_dir" => {
            if args.len() != 3 {
                eprintln!("Error: delete_dir requires dirname");
                1
            } else {
                do_delete_dir(&args[2])
            }
        }
        "create_tmp_file" => {
            if args.len() != 3 {
                eprintln!("Error: create_tmp_file requires filename");
                1
            } else {
                do_create_tmp_file(&args[2])
            }
        }
        _ => {
            eprintln!("Error: unknown operation '{}'", operation);
            print_usage(&args[0]);
            1
        }
    };

    process::exit(exit_code);
}
