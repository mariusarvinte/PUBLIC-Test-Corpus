// © 2026 Massachusetts Institute of Technology
// MIT License

// inventory.h
#ifndef INVENTORY_H
#define INVENTORY_H

#include "generic_containers.h"

#define MAX_NAME_LENGTH 64
#define MAX_CATEGORY_LENGTH 32

typedef struct {
    int id;
    char name[MAX_NAME_LENGTH];
    char category[MAX_CATEGORY_LENGTH];
    double price;
    int quantity;
} item_t;

typedef struct {
    int customer_id;
    char customer_name[MAX_NAME_LENGTH];
    double total_amount;
} order_t;

// Declare containers for different types
DECLARE_ARRAY(int)
DECLARE_ARRAY(double)
DECLARE_ARRAY(item_t)
DECLARE_ARRAY(order_t)

DECLARE_LIST(int)
DECLARE_LIST(double)
DECLARE_LIST(item_t)
DECLARE_LIST(order_t)

// Inventory management functions
void print_item(item_t item);
void print_order(order_t order);
item_t create_item(int id, const char *name, const char *category, 
                   double price, int quantity);
order_t create_order(int customer_id, const char *customer_name, 
                     double total_amount);

// Statistics functions
void calculate_inventory_stats(array_item_t_t *items);
void calculate_order_stats(list_order_t_t *orders);
void find_items_by_category(array_item_t_t *items, const char *category);
void find_expensive_items(list_item_t_t *items, double min_price);

#endif // INVENTORY_H
