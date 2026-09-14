// © 2026 Massachusetts Institute of Technology
// MIT License

#include <stdlib.h>

#include "matrix.h"
#include "write.h"

#include <stdio.h>

#define OUT_FILE "matrix.txt"

int driver(int width_a, int height_a, const char* matrix_a, int width_b, int height_b, const char* matrix_b) {
    matrix_t* mat_a = initialize_matrix_from_string(matrix_a, width_a, height_a);
    if (mat_a == NULL) {
        return EXIT_FAILURE;
    }
    matrix_t* mat_b = initialize_matrix_from_string(matrix_b, width_b, height_b);
    if (mat_b == NULL) {
        free_matrix(mat_a);
        return EXIT_FAILURE;
    }

    matrix_t* res = multiply_matrices(mat_a, mat_b);
    if (res == NULL) {
        free_matrix(mat_a);
        free_matrix(mat_b);
        return EXIT_FAILURE;
    }
    char* res_str = matrix_to_string(res);
    if (res_str == NULL) {
        free_matrix(mat_a);
        free_matrix(mat_b);
        free(res);
        return EXIT_FAILURE;
    }

    int res_write = write_to_file(OUT_FILE, res_str);

    free_matrix(mat_a);
    free_matrix(mat_b);
    free_matrix(res);
    free(res_str);

    if (res_write != 0) {
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
