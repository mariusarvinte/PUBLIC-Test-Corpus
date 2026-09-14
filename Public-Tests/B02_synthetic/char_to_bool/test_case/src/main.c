// © 2026 Massachusetts Institute of Technology
// MIT License

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>

/* External function declaration */
int process_decisions(char *decision_string, size_t length, 
                     int operation, int param);

#define MAX_INPUT_SIZE 1024

int main(void) {
    char input_buffer[MAX_INPUT_SIZE];
    int operation;
    int param;
    int result;
    
    /* Read operation number */
    if (fgets(input_buffer, MAX_INPUT_SIZE, stdin) == NULL) {
        fprintf(stderr, "Error reading operation\n");
        return 1;
    }
    operation = atoi(input_buffer);
    
    /* Read parameter */
    if (fgets(input_buffer, MAX_INPUT_SIZE, stdin) == NULL) {
        fprintf(stderr, "Error reading parameter\n");
        return 1;
    }
    param = atoi(input_buffer);
    
    /* Read decision string */
    if (fgets(input_buffer, MAX_INPUT_SIZE, stdin) == NULL) {
        fprintf(stderr, "Error reading decision string\n");
        return 1;
    }
    
    /* Remove trailing newline if present */
    size_t len = strlen(input_buffer);
    if (len > 0 && input_buffer[len - 1] == '\n') {
        input_buffer[len - 1] = '\0';
        len--;
    }
    
    /* Call the library function */
    result = process_decisions(input_buffer, len, operation, param);
    
    /* Print result to stdout */
    printf("%d\n", result);
    
    return 0;
}
