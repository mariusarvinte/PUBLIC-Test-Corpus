// © 2026 Massachusetts Institute of Technology
// MIT License

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>

#define MAKE_FUNC_NAME(prefix, suffix) prefix##_##suffix
#define LOG_MSG(level, msg) printf("[" #level "] " msg "\n")
#define CREATE_LABEL(name) name##_label

typedef int (*operation_fn)(int value, int unused_param, void *unused_context);

int process_value(int value, int unused_param, void *unused_context);
int double_value(int value, int unused_param, void *unused_context);
int triple_value(int value, int unused_param, void *unused_context);

typedef struct {
    int *results;
    size_t capacity;
    size_t count;
    operation_fn operation;
    char status;
} ProcessorState;

static bool is_valid_state(ProcessorState *state) {
    if (state->status) {
        return state->count < state->capacity;
    }
    return false;
}

static bool check_char_flag(char flag) {
    return flag;
}

int process_value(int value, int unused_param, void *unused_context) {
    (void)unused_param;
    (void)unused_context;
    return value + 10;
}

int double_value(int value, int unused_param, void *unused_context) {
    (void)unused_param;
    (void)unused_context;
    return value * 2;
}

int triple_value(int value, int unused_param, void *unused_context) {
    (void)unused_param;
    (void)unused_context;
    return value * 3;
}

static ProcessorState* init_processor(size_t capacity, operation_fn op) {
    ProcessorState *state = malloc(sizeof(ProcessorState));
    if (!state) {
        return NULL;
    }

    state->results = malloc(capacity * sizeof(int));
    if (!state->results) {
        free(state);
        return NULL;
    }

    state->capacity = capacity;
    state->count = 0;
    state->operation = op;
    state->status = 1;

    return state;
}

static void cleanup_processor(ProcessorState *state) {
    if (state) {
        if (state->results) {
            free(state->results);
        }
        free(state);
    }
}

int gotomach(int iterations, int seed, int mode, int threshold) {
    ProcessorState *state = NULL;
    int *temp_buffer = NULL;
    int result = 0;
    operation_fn selected_op = NULL;

    LOG_MSG(INFO, "Starting gotomach function");

    if (iterations < 0 || iterations > UINT16_MAX) {
        LOG_MSG(ERROR, "Invalid iteration count");
        result = -1;
        goto cleanup;
    }

    if (seed < 0 || seed > UINT16_MAX) {
        LOG_MSG(ERROR, "Invalid seed value");
        result = -2;
        goto cleanup;
    }

    switch (mode) {
        case 0:
            selected_op = process_value;
            break;
        case 1:
            selected_op = double_value;
            break;
        case 2:
            selected_op = triple_value;
            break;
        default:
            LOG_MSG(WARNING, "Invalid mode, using default");
            selected_op = process_value;
            break;
    }

    state = init_processor(iterations, selected_op);
    if (!state) {
        LOG_MSG(ERROR, "Failed to initialize processor");
        result = -3;
        goto cleanup;
    }

    temp_buffer = malloc(iterations * sizeof(int));
    if (!temp_buffer) {
        LOG_MSG(ERROR, "Failed to allocate temporary buffer");
        result = -4;
        goto cleanup;
    }

    if (!check_char_flag(state->status)) {
        LOG_MSG(ERROR, "Invalid state status");
        result = -5;
        goto cleanup;
    }

    int current_value = seed;
    for (int i = 0; i < iterations; i++) {
        if (!is_valid_state(state)) {
            LOG_MSG(ERROR, "State became invalid during processing");
            result = -6;
            goto cleanup;
        }

        temp_buffer[i] = state->operation(current_value, 0, NULL);

        if (temp_buffer[i] < threshold) {
            state->results[state->count++] = temp_buffer[i];
        }

        current_value = temp_buffer[i] % 1000;

        if (state->count >= UINT16_MAX) {
            LOG_MSG(WARNING, "Reached maximum count");
            break;
        }
    }

    result = 0;
    for (size_t i = 0; i < state->count; i++) {
        result += state->results[i];
    }

    LOG_MSG(INFO, "Processing completed successfully");

cleanup:
    if (temp_buffer) {
        free(temp_buffer);
    }
    cleanup_processor(state);

    return result;
}

