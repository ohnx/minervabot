#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "module.h"

struct core_ctx *ctx;

#define SLEEPCMD "sleep"
#define SEGCMD "segfault"

int handle_sleep(const char *cmdname, struct command_sender who, char *where, char *args) {
    ctx->log(INFO, "sleep", "[cmdname: %s] [where: %s] [who: %s!%s@%s] [args: %s]", cmdname, where, who.nick, who.ident, who.host, args);
    usleep(10000000);
    ctx->msg(where, "Zzz... done!");
    return 0;
}

int handle_segfault(const char *cmdname, struct command_sender who, char *where, char *args) {
    int *n = NULL;
    n += *n;
    return *n;
}


int module_init(struct core_ctx *core) {
    ctx = core;
    return ctx->register_cmd(SLEEPCMD, &handle_sleep) + ctx->register_cmd(SEGCMD, &handle_segfault);
}

void module_cleanup() {
    ctx->unregister_cmd(SLEEPCMD);
    ctx->unregister_cmd(SEGCMD);
}
