// © 2026 Massachusetts Institute of Technology
// MIT License

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "logger.h"

static FILE *log_file = NULL;

int initialize_logger() {
    const char *log_file_env = getenv("LOG_FILE");
    const char *log_file_path = log_file_env ? log_file_env : "default.log";

    log_file = fopen(log_file_path, "a");
    if (!log_file) {
        fprintf(stderr, "Failed to open log file: %s\n", log_file_path);
        return -1;
    }

    log_info("Logger initialized.");
    return 0;
}

void log_info(const char *message) {
    if (log_file) {
        fprintf(log_file, "[INFO] %s\n", message);
    }
}

void log_warning(const char *message) {
    if (log_file) {
        fprintf(log_file, "[WARNING] %s\n", message);
    }
}

void log_error(const char *message) {
    if (log_file) {
        fprintf(log_file, "[ERROR] %s\n", message);
    }
}

void finalize_logger() {
    if (log_file) {
        log_info("Logger finalized.");
        fclose(log_file);
    }
}
