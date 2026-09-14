// © 2026 Massachusetts Institute of Technology
// MIT License

// scene.h
#ifndef SCENE_H
#define SCENE_H

#include "shape.h"

#define MAX_SHAPES_IN_SCENE 50
#define MAX_SCENE_NAME 64

typedef struct {
    char name[MAX_SCENE_NAME];
    shape_t *shapes[MAX_SHAPES_IN_SCENE];
    int shape_count;
} scene_t;

// Create a new empty scene
scene_t* scene_create(const char *name);

// Destroy a scene
void scene_destroy(scene_t *scene);

// Add a shape to the scene
int scene_add_shape(scene_t *scene, shape_t *shape);

// Remove a shape at index
int scene_remove_shape(scene_t *scene, int index);

// Print the scene
void scene_print(const scene_t *scene);

// Compare two scenes for equality (1:1 correspondence)
int scene_equals(const scene_t *s1, const scene_t *s2);

// Save scene to file
int scene_save(const scene_t *scene, const char *filename);

// Load scene from file
scene_t* scene_load(const char *filename);

// List all shapes in scene
void scene_list_shapes(const scene_t *scene);

#endif // SCENE_H
