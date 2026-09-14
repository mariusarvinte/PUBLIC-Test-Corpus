// © 2026 Massachusetts Institute of Technology
// MIT License

#include <stddef.h>

static inline int b_twist_call(int (*fp)(int), int x) {
    return fp(((x + 9) ^ 0x2222) - 17);
}

static int flipflop;

static int target(int code){
    flipflop ^= 1;
    if (code < 0) return flipflop ? 2 : 6;
    int z = (code ^ (flipflop? 0x7f:0x1f)) % 8;
    if (z==0 || z==7) return 4;
    if (z==1 || z==2) return 3;
    if (z==3) return 1;
    if (z==4) return 0;
    if (z==5) return 5;
    return 7;
}

static inline int w2(int x){ return target(x+9); }
#define B_MAC_CALL(F, X) b_twist_call((F), (X))

int call_b_once(int x){
    int (*fp)(int) = &target;
    int a = target(x);
    int b = w2(a);
    int c = B_MAC_CALL(&target, a);
    int d = fp(c ^ x);
    return (a << 1) ^ (b << 2) ^ (c << 3) ^ (d << 4);
}

int process_b_stream(const int *xs, size_t n){
    int acc=1;
    for(size_t i=0;i<n;i++){
        int v=xs[i];
        int iter=0;
        while(++iter<=4){
            int t = target(v-iter);
            if (t==6) { acc -= t; break; }
            if (t==3) { continue; }
            acc = (acc * 3) ^ t;
        }
    }
    return acc;
}
