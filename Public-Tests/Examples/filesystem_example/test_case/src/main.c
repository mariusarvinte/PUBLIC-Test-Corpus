// © 2026 Massachusetts Institute of Technology
// MIT License

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>

// Write content to a file
int do_write(const char* filename, const char* content) {
    FILE* file = fopen(filename, "w");
    if (!file) {
        fprintf(stderr, "Write failed\n");
        return -1;
    }
    
    size_t len = strlen(content);
    size_t written = fwrite(content, 1, len, file);
    fclose(file);
    
    if (written != len) {
        fprintf(stderr, "Write failed\n");
        return -1;
    }
    
    printf("File written successfully\n");
    return 0;
}

// Read file contents and print to stdout
int do_read(const char* filename) {
    FILE* file = fopen(filename, "r");
    if (!file) {
        fprintf(stderr, "File not found\n");
        return -1;
    }
    
    // Get file size
    fseek(file, 0, SEEK_END);
    long size = ftell(file);
    fseek(file, 0, SEEK_SET);
    
    // Allocate buffer dynamically
    char* buffer = (char*)malloc(size + 1);
    if (!buffer) {
        fprintf(stderr, "Read failed\n");
        fclose(file);
        return -1;
    }
    
    size_t read_count = fread(buffer, 1, size, file);
    buffer[read_count] = '\0';
    fclose(file);
    
    printf("%s", buffer);
    free(buffer);
    return 0;
}

// Append content to a file
int do_append(const char* filename, const char* content) {
    FILE* file = fopen(filename, "a");
    if (!file) {
        fprintf(stderr, "Append failed\n");
        return -1;
    }
    
    size_t len = strlen(content);
    size_t written = fwrite(content, 1, len, file);
    fclose(file);
    
    if (written != len) {
        fprintf(stderr, "Append failed\n");
        return -1;
    }
    
    printf("Content appended successfully\n");
    return 0;
}

// Create a file
int do_create(const char* filename) {
    FILE* file = fopen(filename, "w");
    if (!file) {
        fprintf(stderr, "File creation failed\n");
        return -1;
    }
    
    fclose(file);
    
    printf("File created successfully\n");
    return 0;
}

// Delete a file
int do_delete(const char* filename) {
    if (remove(filename) == 0) {
        printf("File deleted successfully\n");
        return 0;
    } else {
        fprintf(stderr, "Delete failed\n");
        return -1;
    }
}

// Get environment variable
int do_getenv(const char* varname) {
    const char* value = getenv(varname);
    if (value) {
        printf("%s\n", value);
        return 0;
    } else {
        printf("(not set)\n");
        return 1;
    }
}

// Set environment variable
int do_setenv(const char* varname, const char* value) {
    if (setenv(varname, value, 1) == 0) {
        printf("Environment variable set\n");
        return 0;
    } else {
        fprintf(stderr, "Setenv failed\n");
        return -1;
    }
}

// Change file permissions
int do_chmod(const char* filename) {
    // Hardcoded permissions: 0644 (rw-r--r--)
    mode_t mode = 0644;
    
    if (chmod(filename, mode) == 0) {
        printf("Permissions changed successfully\n");
        return 0;
    } else {
        fprintf(stderr, "Chmod failed\n");
        return -1;
    }
}

// Create a directory
int do_mkdir(const char* dirname) {
    if (mkdir(dirname, 0755) == 0) {
        printf("Directory created successfully\n");
        return 0;
    } else {
        fprintf(stderr, "Mkdir failed\n");
        return -1;
    }
}

// Rename a file or directory
int do_rename(const char* old_path, const char* new_path) {
    if (rename(old_path, new_path) == 0) {
        printf("Path renamed successfully\n");
        return 0;
    } else {
        fprintf(stderr, "Rename failed\n");
        return -1;
    }
}

// Delete an empty directory
int do_delete_dir(const char* dirname) {
    if (rmdir(dirname) == 0) {
        printf("Directory deleted successfully\n");
        return 0;
    } else {
        fprintf(stderr, "Delete directory failed\n");
        return -1;
    }
}

// Create and delete a temporary file
int do_create_tmp_file(const char* filename) {
    // Create the file
    FILE* file = fopen(filename, "w");
    if (!file) {
        fprintf(stderr, "Temporary file creation failed\n");
        return -1;
    }
    fclose(file);
    printf("Temporary file created\n");
    
    // Delete the file
    if (remove(filename) == 0) {
        printf("Temporary file deleted\n");
        return 0;
    } else {
        fprintf(stderr, "Temporary file deletion failed\n");
        return -1;
    }
}

void print_usage(const char* progname) {
    fprintf(stderr, "Usage: %s <operation> <args...>\n", progname);
    fprintf(stderr, "Operations:\n");
    fprintf(stderr, "  write <filename> <content>\n");
    fprintf(stderr, "  read <filename>\n");
    fprintf(stderr, "  append <filename> <content>\n");
    fprintf(stderr, "  create <filename> <content>\n");
    fprintf(stderr, "  delete <filename>\n");
    fprintf(stderr, "  getenv <varname>\n");
    fprintf(stderr, "  setenv <varname> <value>\n");
    fprintf(stderr, "  chmod <filename>\n");
    fprintf(stderr, "  mkdir <dirname>\n");
    fprintf(stderr, "  rename <old_path> <new_path>\n");
    fprintf(stderr, "  delete_dir <dirname>\n");
    fprintf(stderr, "  create_tmp_file <filename>\n");
}

int main(int argc, char** argv) {
    if (argc < 2) {
        print_usage(argv[0]);
        return 1;
    }
    
    const char* operation = argv[1];
    
    if (strcmp(operation, "write") == 0) {
        if (argc != 4) {
            fprintf(stderr, "Error: write requires filename and content\n");
            return 1;
        }
        return do_write(argv[2], argv[3]);
    }
    else if (strcmp(operation, "read") == 0) {
        if (argc != 3) {
            fprintf(stderr, "Error: read requires filename\n");
            return 1;
        }
        return do_read(argv[2]);
    }
    else if (strcmp(operation, "append") == 0) {
        if (argc != 4) {
            fprintf(stderr, "Error: append requires filename and content\n");
            return 1;
        }
        return do_append(argv[2], argv[3]);
    }
    else if (strcmp(operation, "create") == 0) {
        if (argc != 3) {
            fprintf(stderr, "Error: create requires filename\n");
            return 1;
        }
        return do_create(argv[2]);
    }
    else if (strcmp(operation, "delete") == 0) {
        if (argc != 3) {
            fprintf(stderr, "Error: delete requires filename\n");
            return 1;
        }
        return do_delete(argv[2]);
    }
    else if (strcmp(operation, "getenv") == 0) {
        if (argc != 3) {
            fprintf(stderr, "Error: getenv requires varname\n");
            return 1;
        }
        return do_getenv(argv[2]);
    }
    else if (strcmp(operation, "setenv") == 0) {
        if (argc != 4) {
            fprintf(stderr, "Error: setenv requires varname and value\n");
            return 1;
        }
        return do_setenv(argv[2], argv[3]);
    }
    else if (strcmp(operation, "chmod") == 0) {
        if (argc != 3) {
            fprintf(stderr, "Error: chmod requires filename\n");
            return 1;
        }
        return do_chmod(argv[2]);
    }
    else if (strcmp(operation, "mkdir") == 0) {
        if (argc != 3) {
            fprintf(stderr, "Error: mkdir requires dirname\n");
            return 1;
        }
        return do_mkdir(argv[2]);
    }
    else if (strcmp(operation, "rename") == 0) {
        if (argc != 4) {
            fprintf(stderr, "Error: rename requires old_path and new_path\n");
            return 1;
        }
        return do_rename(argv[2], argv[3]);
    }
    else if (strcmp(operation, "delete_dir") == 0) {
        if (argc != 3) {
            fprintf(stderr, "Error: delete_dir requires dirname\n");
            return 1;
        }
        return do_delete_dir(argv[2]);
    }
    else if (strcmp(operation, "create_tmp_file") == 0) {
        if (argc != 3) {
            fprintf(stderr, "Error: create_tmp_file requires filename\n");
            return 1;
        }
        return do_create_tmp_file(argv[2]);
    }
    else {
        fprintf(stderr, "Error: unknown operation '%s'\n", operation);
        print_usage(argv[0]);
        return 1;
    }
}
