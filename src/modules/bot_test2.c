#include "module.h"

int module_init(struct core_ctx *core) {
    int *a = (int *)0x0;
    a += 2;
    return *a;
}

void module_cleanup() {
    int a = 6;
    int b = 3-3;
    int c = a/b;
    a = c;
}
