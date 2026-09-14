// © 2026 Massachusetts Institute of Technology
// MIT License

#include <stdio.h>
#include <stdlib.h>
#include "task_manager.h"
#include "logger.h"
#include <string.h>

int driver(const char *tasks) {
    int res = initialize_logger();
    if (res != 0) {
        return EXIT_FAILURE;
    }

    TaskManager *manager = create_task_manager();
    if (!manager) {
        return EXIT_FAILURE;
    }

    const char *start = tasks;
    int priority = 1;
    while (*start != '\0') {
        const char *end = strchr(start, '\n');
        if (end == NULL) {
            end = start + strlen(start);
        }

        // Extract the current task
        size_t length = end - start;
        char *task = (char *)malloc(length + 1);
        if (!task) {
            fprintf(stderr, "Error: Failed to allocate memory for task.\n");
            destroy_task_manager(manager);
            finalize_logger();
            return EXIT_FAILURE;
        }
        strncpy(task, start, length);
        task[length] = '\0';

        add_task(manager, task, priority++);
        free(task);
        start = (*end == '\n') ? end + 1 : end;
    }    

    print_tasks(manager);

    destroy_task_manager(manager);
    finalize_logger();

    return 0;
}
