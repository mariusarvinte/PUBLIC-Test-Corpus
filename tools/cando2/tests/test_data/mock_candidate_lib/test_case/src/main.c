// © 2026 Massachusetts Institute of Technology
// MIT License

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "lib.h"

int main(int argc, char* argv[]) {
    if (argc == 1) {
        printf("Hello, world!\n");
    } else if (strcmp(argv[1], "1") == 0) {
        read_write();
    } else if (strcmp(argv[1], "6") == 0) {
        const char* value = getenv("CANDO_TEST_VECTOR_ENV");
        printf("%s\n", value == NULL ? "<unset>" : value);
    } else if (strcmp(argv[1], "3") == 0) {
        segfault();
    } else if (strcmp(argv[1], "4") == 0) {
        return 1;
    } else if (strcmp(argv[1], "5") == 0) {
        timeout();
    }
}
