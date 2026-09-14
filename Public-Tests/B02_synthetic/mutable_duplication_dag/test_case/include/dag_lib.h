// © 2026 Massachusetts Institute of Technology
// MIT License

// dag_lib.h
#ifndef DAG_LIB_H
#define DAG_LIB_H

#include <stddef.h>

#define MAX_CITY_NAME 64
#define MAX_EDGES 10
#define MAX_NODES 100

typedef struct node_t node_t;
typedef struct edge_t edge_t;
typedef struct graph_t graph_t;

struct edge_t {
    node_t *destination;
    int distance;
};

struct node_t {
    char city_name[MAX_CITY_NAME];
    int ref_count;
    edge_t edges[MAX_EDGES];
    int edge_count;
};

struct graph_t {
    node_t *nodes[MAX_NODES];
    int node_count;
};


// Create a new empty graph
graph_t* create_graph(void);

// Add a node to the graph (returns pointer to node)
node_t* add_node(graph_t *graph, const char *city_name);

// Add an edge between two nodes with a given distance
int add_edge(node_t *from, node_t *to, int distance);

// Delete a node (decrements ref count, frees if 0)
void delete_node(node_t *node);

// Create shallow copy of subsection starting from node (increments ref counts)
node_t* shallow_copy(node_t *start);

// Find shortest path between two nodes (returns array of nodes, NULL if no path)
node_t** find_shortest_path(node_t *start, node_t *end, int *path_length);

// Free the entire graph
void free_graph(graph_t *graph);

// Get node by city name
node_t* get_node_by_name(graph_t *graph, const char *city_name);

// Print node information (for debugging)
void print_node(node_t *node);

// Print entire graph
void print_graph(graph_t *graph);

#endif // DAG_LIB_H
