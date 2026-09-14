// © 2026 Massachusetts Institute of Technology
// MIT License

#ifndef TASK_MANAGER_H
#define TASK_MANAGER_H

typedef struct {
    char description[256];
    int priority;
} Task;

typedef struct {
    Task *tasks;
    int max_tasks;
    int task_count;
} TaskManager;

TaskManager *create_task_manager();
void add_task(TaskManager *manager, const char *description, int priority);
void print_tasks(const TaskManager *manager);
void destroy_task_manager(TaskManager *manager);

#endif // TASK_MANAGER_H
