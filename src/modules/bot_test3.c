#include "module.h"

int module_init(struct core_ctx *core) {
    return 0;
}

void module_cleanup() {
    int a = 6;
    int *b = (int *)0x0;
    int c = a/(*b);
    a = c;
}
