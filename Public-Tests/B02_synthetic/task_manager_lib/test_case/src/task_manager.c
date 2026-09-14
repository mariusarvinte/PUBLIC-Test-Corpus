// © 2026 Massachusetts Institute of Technology
// MIT License

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "task_manager.h"
#include "logger.h"

TaskManager *create_task_manager() {
    TaskManager *manager = (TaskManager *)malloc(sizeof(TaskManager));
    if (!manager) {
        log_error("Failed to allocate memory for TaskManager.");
        return NULL;
    }

    const char *max_tasks_env = getenv("MAX_TASKS");
    manager->max_tasks = max_tasks_env ? atoi(max_tasks_env) : 10;
    manager->task_count = 0;
    manager->tasks = (Task *)malloc(manager->max_tasks * sizeof(Task));
    if (!manager->tasks) {
        log_error("Failed to allocate memory for tasks.");
        free(manager);
        return NULL;
    }

    log_info("TaskManager created successfully.");
    return manager;
}

void add_task(TaskManager *manager, const char *description, int priority) {
    if (manager->task_count >= manager->max_tasks) {
        log_warning("Cannot add task: Maximum task limit reached.");
        return;
    }

    Task *task = &manager->tasks[manager->task_count++];
    strncpy(task->description, description, sizeof(task->description) - 1);
    task->description[sizeof(task->description) - 1] = '\0';
    task->priority = priority;

    log_info("Task added successfully.");
}

void print_tasks(const TaskManager *manager) {
    printf("Tasks:\n");
    for (int i = 0; i < manager->task_count; i++) {
        printf("  [%d] %s (Priority: %d)\n", i + 1, manager->tasks[i].description, manager->tasks[i].priority);
    }
}

void destroy_task_manager(TaskManager *manager) {
    free(manager->tasks);
    free(manager);
    log_info("TaskManager destroyed successfully.");
}
