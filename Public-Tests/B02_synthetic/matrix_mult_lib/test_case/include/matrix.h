// © 2026 Massachusetts Institute of Technology
// MIT License

typedef struct {
    int** matrix;
    int width;
    int height;
} matrix_t;

matrix_t* initialize_matrix_from_string(const char* input, int width, int height);
void free_matrix(matrix_t* mat);
matrix_t* multiply_matrices(matrix_t* mat_a, matrix_t* mat_b);
char* matrix_to_string(matrix_t* mat);
