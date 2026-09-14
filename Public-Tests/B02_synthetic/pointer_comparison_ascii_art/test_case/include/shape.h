// © 2026 Massachusetts Institute of Technology
// MIT License

#ifndef SHAPE_H
#define SHAPE_H

#include <stddef.h>

#define MAX_SHAPE_WIDTH 80
#define MAX_SHAPE_HEIGHT 30
#define MAX_SHAPE_NAME 32

typedef enum {
    SHAPE_TREE,
    SHAPE_TRACTOR,
    SHAPE_HOUSE,
    SHAPE_SUN,
    SHAPE_CLOUD,
    SHAPE_FLOWER,
    SHAPE_CAR,
    SHAPE_STAR,
    SHAPE_HEART,
    SHAPE_RAINBOW,
    SHAPE_COUNT
} shape_type_t;

typedef struct {
    shape_type_t type;
    char name[MAX_SHAPE_NAME];
    char art[MAX_SHAPE_HEIGHT][MAX_SHAPE_WIDTH];
    int width;
    int height;
} shape_t;

// Initialize the shape manager (allocate all shapes once)
void shape_manager_init(void);

// Clean up shape manager
void shape_manager_cleanup(void);

// Get a shape by type (returns singleton instance)
shape_t* shape_get(shape_type_t type);

// Print a shape to stdout
void shape_print(const shape_t *shape);

// Compare two shapes (equal if XOR of pointers is 0)
int shape_equals(const shape_t *s1, const shape_t *s2);

// Get shape type name
const char* shape_type_name(shape_type_t type);

#endif // SHAPE_H
