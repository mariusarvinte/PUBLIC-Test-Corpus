// © 2026 Massachusetts Institute of Technology
// MIT License

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int mock_candidate(int input) {
	return input + 3;
}

int add_three(int input) {
	return input + 3;
}

void read_write() {
    char input[8];

    // Prompt the user for input
    printf("Please enter some text: ");
    fflush(stdout); // Ensure the prompt is displayed immediately

    // Read input from stdin, ensuring it does not exceed the buffer size
    if (fgets(input, sizeof(input), stdin) != NULL) {
        // Remove the trailing newline character, if present
        size_t len = strlen(input);
        if (len > 0 && input[len - 1] == '\n') {
            input[len - 1] = '\0';
        }

        // Write the sanitized input to stdout
        printf("You entered: %s\n", input);
    } else {
        // Handle input error
        fprintf(stderr, "Error reading input.\n");
    }

    fprintf(stderr, "Printing to stderr\n");
}

// Purposefully dereference null pointer to cause segfault
void segfault() {
    sleep(2);
    printf("Segfaulting...\n");
    int *ptr = NULL;
    *ptr = 42;
}

void timeout() {
    sleep(10);
    printf("This should never be printed");
}

double add_float(double a, double b) {
    return a + b;
}

int conduct_me(int a, int b) {
    if (a < 0) {
        read_write();
    } else if (a == 6) {
        const char* value = getenv("CANDO_TEST_VECTOR_ENV");
        printf("%s\n", value == NULL ? "<unset>" : value);
    } else if (a == 42 ) {
        segfault();
    } else if (a == 2) {
        timeout();
    } else {
        printf("Hello World\n");
    }

	return a + b;
}
