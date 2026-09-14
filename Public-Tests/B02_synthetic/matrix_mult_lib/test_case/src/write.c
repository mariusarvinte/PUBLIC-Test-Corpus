// © 2026 Massachusetts Institute of Technology
// MIT License

#include <stdio.h>
#include <errno.h>
#include <string.h>

#include "write.h"

int write_to_file(const char* filename, const char* content) {
    if (content == NULL) {
        fprintf(stderr, "Error: Content is NULL.\n");
        return EINVAL;
    }

    FILE* file = fopen(filename, "w");
    if (file == NULL) {
        fprintf(stderr, "Error opening file '%s': %s\n", filename, strerror(errno));
        return errno;
    }

    if (fprintf(file, "%s", content) < 0) {
        fprintf(stderr, "Error writing to file '%s': %s\n", filename, strerror(errno));
        fclose(file);
        return errno;
    }

    if (fclose(file) != 0) {
        fprintf(stderr, "Error closing file '%s': %s\n", filename, strerror(errno));
        return errno;
    }

    return 0;
}
