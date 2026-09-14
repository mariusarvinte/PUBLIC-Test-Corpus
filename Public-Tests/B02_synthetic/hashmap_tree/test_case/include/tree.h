// © 2026 Massachusetts Institute of Technology
// MIT License

// tree.h
#ifndef TREE_H
#define TREE_H

#include "hashmap.h"

#define MAX_CHILDREN 32
#define MAX_DATA_LENGTH 256

typedef struct tree_node {
    tree_id_t id;
    tree_id_t parent_id;
    tree_id_t child_ids[MAX_CHILDREN];
    int child_count;
    char data[MAX_DATA_LENGTH];
} tree_node_t;

typedef struct {
    hashmap_t *node_map;
    tree_id_t root_id;
    int has_root;
    size_t node_count;
} tree_t;

// Create a new tree
tree_t* tree_create(void);

// Delete entire tree and free all nodes
void tree_delete(tree_t *tree);

// Add a node to the tree
int tree_add_node(tree_t *tree, tree_id_t id, tree_id_t parent_id, const char *data);

// Remove a node and all its descendants
int tree_remove_node(tree_t *tree, tree_id_t id);

// Get node by ID
tree_node_t* tree_get_node(tree_t *tree, tree_id_t id);

// Check if node exists
int tree_contains(tree_t *tree, tree_id_t id);

// Get tree size
size_t tree_size(tree_t *tree);

// Print tree structure
void tree_print(tree_t *tree);

// Get depth of a node
int tree_get_depth(tree_t *tree, tree_id_t id);

// Get height of tree from a node
int tree_get_height(tree_t *tree, tree_id_t id);

// Count descendants of a node
int tree_count_descendants(tree_t *tree, tree_id_t id);

// Find path from root to node
int tree_find_path(tree_t *tree, tree_id_t id, tree_id_t *path, int max_length);

#endif // TREE_H
