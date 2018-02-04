#include <stdio.h>
#include "module.h"

struct core_ctx *ctx;

#define HELLOCMD "hello"

int handle_hello(const char *cmdname, char *where, char *who, char *args) {
    ctx->log(INFO, "hello", "[cmdname: %s] [where: %s] [who: %s] [args: %s]\n", cmdname, where, who, args);
    ctx->msg(where, "hello world!");
    return 0;
}

int module_init(struct core_ctx *core) {
    ctx = core;
    return ctx->register_cmd(HELLOCMD, &handle_hello);
}

void module_cleanup() {
    ctx->unregister_cmd(HELLOCMD);
}
