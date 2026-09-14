// © 2026 Massachusetts Institute of Technology
// MIT License

#include <stdio.h>
#include "mdmacros.h"

/* Define operations */
int op_add(int a,int b){ return a + b; }
int op_sub(int a,int b){ return a - b; }
int op_mul(int a,int b){ return a * b; }

/* Define the macro-generated accumulator for the selected OP */
DEFINE_ACCUM(OP)

/* Global macro uses at file scope (exercises expansion at global init) */
int (*G_OP)(int,int) = OP_FN(OP);
const char *G_OP_NAME = STR(OP);

int helper_call(int a, int b) {
    int r = (OP_FN(OP))(a, b);
    int acc = INIT_FOR(OP);
    RUN_LOOP(OP, acc, REPEAT);
    printf("helper.call=%d helper.acc=%d\n", r, acc);
    return r + acc;
}

int helper_ptr(int a, int b) {
    int (*fp)(int,int) = OP_FN(OP);
    int r = fp(a, b);
    printf("helper.ptr=%d\n", r);
    return r;
}

int use_generated(int n) {
    int r = (ACCUM_FN(OP))(n);
    printf("gen.acc=%d\n", r);
    return r;
}
