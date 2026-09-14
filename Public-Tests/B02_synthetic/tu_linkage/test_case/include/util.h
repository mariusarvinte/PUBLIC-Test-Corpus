// © 2026 Massachusetts Institute of Technology
// MIT License

#ifndef UTIL_H
#define UTIL_H
#include <stddef.h>
#include <stdbool.h>
#include <stdio.h>

typedef struct {
    int *data; size_t len, cap;
} IntVec;

void iv_init(IntVec *v);
void iv_free(IntVec *v);
bool iv_reserve(IntVec *v, size_t need);
bool iv_push(IntVec *v, int x);
bool iv_pop(IntVec *v, int *out);
int  iv_peek(const IntVec *v, int def);

typedef struct {
    const int *code; size_t n; size_t ip;
} Program;

void prog_init(Program *p, const int *code, size_t n);
bool prog_fetch(Program *p, int *out);

typedef struct {
    IntVec stack;
    IntVec trace;
    int    steps;
} VM;

void vm_init(VM *vm);
void vm_free(VM *vm);
void vm_trace(VM *vm, int t);
void vm_print(FILE *fp, const char *label, const VM *vm);

int run_engine(int impl_id, const int *code, size_t n, VM *vm);

#endif
