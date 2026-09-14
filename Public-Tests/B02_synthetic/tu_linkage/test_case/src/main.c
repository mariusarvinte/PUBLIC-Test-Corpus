// © 2026 Massachusetts Institute of Technology
// MIT License

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "../include/util.h"
#include "../include/api.h"

int run_engine(int impl_id, const int *code, size_t n, VM *vm);

static void usage(const char *p){
    fprintf(stderr, "Usage: %s [--stdin] [bytecodes...]\n"
                    "Bytecodes are integers forming a small VM program.\n", p);
}

static size_t read_stdin(IntVec *v){
    char buf[4096];
    size_t count=0;
    while (fgets(buf, sizeof buf, stdin)) {
        char *p = buf;
        while (*p) {
            char *q=p;
            while (*q && *q!=' ' && *q!='\t' && *q!='\n' && *q!='\r') ++q;
            char save=*q; *q='\0';
            if (*p) {
                char *e=NULL; long t=strtol(p,&e,10);
                if (e && *e=='\0') { iv_push(v,(int)t); count++; }
            }
            *q=save;
            p = (*q? q+1 : q);
        }
    }
    return count;
}

int main(int argc, char **argv){
    bool use_stdin=false;
    IntVec code; iv_init(&code);

    for (int i=1;i<argc;i++){
        if (!strcmp(argv[i],"--help")) { usage(argv[0]); iv_free(&code); return 0; }
        else if (!strcmp(argv[i],"--stdin")) use_stdin=true;
        else {
            char *e=NULL; long t=strtol(argv[i],&e,10);
            if (e && *e=='\0') iv_push(&code,(int)t);
            else fprintf(stderr,"skip '%s'\n", argv[i]);
        }
    }
    if (use_stdin) read_stdin(&code);
    if (code.len==0) {
        fprintf(stderr,"no program\n");
        iv_free(&code); return 2;
    }

    VM vmA, vmB, vmE;
    vm_init(&vmA); vm_init(&vmB); vm_init(&vmE);

    int rcA = run_engine(0, code.data, code.len, &vmA);
    int rcB = run_engine(1, code.data, code.len, &vmB);
    int rcE = run_engine(2, code.data, code.len, &vmE);

    printf("RC:A=%d B=%d EXT=%d\n", rcA, rcB, rcE);
    vm_print(stdout, "A:", &vmA);
    vm_print(stdout, "B:", &vmB);
    vm_print(stdout, "EXT:", &vmE);

    vm_free(&vmA); vm_free(&vmB); vm_free(&vmE);
    iv_free(&code);
    return 0;
}
