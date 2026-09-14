// © 2026 Massachusetts Institute of Technology
// MIT License

#include <stdio.h>
#include <stdlib.h>
#include "mdmacros.h"

int main(int argc, char **argv) {
    if (argc < 3) {
        fprintf(stderr, "usage: %s A B\n", argv[0]);
        return 2;
    }
    int a = atoi(argv[1]);
    int b = atoi(argv[2]);

    int r_call = (OP_FN(OP))(a, b);
    int acc = INIT_FOR(OP);
    RUN_LOOP(OP, acc, REPEAT);

    int x1 = helper_call(a, b);
    int x2 = helper_ptr(a, b);
    int x3 = use_generated(REPEAT);
    int g  = G_OP(a, b);

    printf("op=%s call=%d acc=%d g.call=%d\n", G_OP_NAME, r_call, acc, g);
    printf("summary=%d\n", r_call + acc + x1 + x2 + x3 + g);
    return 0;
}
