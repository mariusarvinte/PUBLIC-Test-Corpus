// © 2026 Massachusetts Institute of Technology
// MIT License

// hashmap.h
#ifndef HASHMAP_H
#define HASHMAP_H

#include <stddef.h>
#include <stdint.h>

#define HASHMAP_INITIAL_CAPACITY 16
#define HASHMAP_LOAD_FACTOR 0.75

typedef uint64_t tree_id_t;

typedef struct hashmap_entry {
    tree_id_t key;
    void *value;
    int occupied;
    int deleted;
} hashmap_entry_t;

typedef struct {
    hashmap_entry_t *entries;
    size_t capacity;
    size_t size;
    size_t deleted_count;
} hashmap_t;

// Create a new hashmap
hashmap_t* hashmap_create(void);

// Destroy hashmap (does not free values)
void hashmap_destroy(hashmap_t *map);

// Insert or update a key-value pair
int hashmap_put(hashmap_t *map, tree_id_t key, void *value);

// Get value by key (returns NULL if not found)
void* hashmap_get(hashmap_t *map, tree_id_t key);

// Remove a key-value pair (returns the value or NULL)
void* hashmap_remove(hashmap_t *map, tree_id_t key);

// Check if key exists
int hashmap_contains(hashmap_t *map, tree_id_t key);

// Get current size
size_t hashmap_size(hashmap_t *map);

// Clear all entries
void hashmap_clear(hashmap_t *map);

#endif // HASHMAP_H
