// © 2026 Massachusetts Institute of Technology
// MIT License

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

/* External function declaration */
size_t process_buffer(uint8_t *buffer, size_t length, uint32_t flags, 
                     int param1, int param2);

int main(void) {
    uint32_t flags;
    int param1, param2;
    size_t length;
    uint8_t buffer[256];
    
    /* Read flags */
    if (scanf("%u", &flags) != 1) {
        fprintf(stderr, "Error reading flags\n");
        return 1;
    }
    
    /* Read param1 */
    if (scanf("%d", &param1) != 1) {
        fprintf(stderr, "Error reading param1\n");
        return 1;
    }
    
    /* Read param2 */
    if (scanf("%d", &param2) != 1) {
        fprintf(stderr, "Error reading param2\n");
        return 1;
    }
    
    /* Read buffer length */
    if (scanf("%zu", &length) != 1) {
        fprintf(stderr, "Error reading length\n");
        return 1;
    }
    
    if (length > 256) {
        fprintf(stderr, "Error: length %zu exceeds maximum 256\n", length);
        return 1;
    }
    
    /* Read buffer data */
    for (size_t i = 0; i < length; i++) {
        unsigned int byte;
        if (scanf("%u", &byte) != 1) {
            fprintf(stderr, "Error reading byte %zu\n", i);
            return 1;
        }
        buffer[i] = (uint8_t)byte;
    }
    
    /* Process the buffer */
    size_t new_length = process_buffer(buffer, length, flags, param1, param2);
    
    /* Output new length */
    printf("%zu", new_length);
    
    /* Output buffer contents */
    for (size_t i = 0; i < new_length; i++) {
        printf(" %u", buffer[i]);
    }
    printf("\n");
    
    return 0;
}
