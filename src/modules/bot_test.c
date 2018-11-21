#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "module.h"

struct core_ctx *ctx;

#define SLEEPCMD "sleep"
#define SEGCMD "segfault"
#define DIVZERO "divzero"
#define CALLBAD "callbad"
#define SPAMCMD "spam"

int handle_sleep(const char *cmdname, struct command_sender who, char *where, char *args) {
    usleep(10000000);
    ctx->msg(where, "Zzz... done!");
    return 0;
}

int handle_segfault(const char *cmdname, struct command_sender who, char *where, char *args) {
    int *n = NULL;
    n += *n;
    return *n;
}

int handle_divzero(const char *cmdname, struct command_sender who, char *where, char *args) {
    int a = 10;
    int b = 0;
    a = a/b;
    return a/b;
}

int handle_spam(const char *cmdname, struct command_sender who, char *where, char *args) {
    if (who.permission_level < PERMS_OWNER) {
        ctx->msgva(where, "%s: Insufficient permissions", who.nick);
        return 0;
    }

    ctx->msg(where, "spam spam spam spam");
    ctx->msg(where, "spam spam spam spam");
    ctx->msg(where, "spam spam spam spam");
    ctx->msg(where, "spam spam spam spam");
    ctx->msg(where, "spam spam spam spam");
    ctx->msg(where, "spam spam spam spam");
    ctx->msg(where, "spam spam spam spam");
    ctx->msg(where, "spam spam spam spam");
    ctx->msg(where, "spam spam spam spam");
    ctx->msg(where, "spam spam spam spam");
    ctx->msg(where, "spam spam spam spam");
    ctx->msg(where, "spam spam spam spam");
    ctx->msg(where, "spam spam spam spam");
    ctx->msg(where, "spam spam spam spam");
    ctx->msg(where, "spam spam spam spam");
    ctx->msg(where, "spam spam spam spam");
    ctx->msg(where, "spam spam spam spam");
    ctx->msg(where, "spam spam spam spam");
    ctx->msg(where, "spam spam spam spam");
    ctx->msg(where, "spam spam spam spam");
    ctx->msg(where, "spam spam spam spam");
    ctx->msg(where, "spam spam spam spam");
    ctx->msg(where, "spam spam spam spam");
    ctx->msg(where, "spam spam spam spam");
    ctx->msg(where, "spam spam spam spam");
    ctx->msg(where, "spam spam spam spam");
    ctx->msg(where, "spam spam spam spam");
    ctx->msg(where, "spam spam spam spam");
    ctx->msg(where, "spam spam spam spam");
    ctx->msg(where, "spam spam spam spam");
    usleep(11000000);
    ctx->msg(where, "Zzz... done!");
    return 0;
}

int module_init(struct core_ctx *core) {
    ctx = core;
    usleep(3000000);
    return ctx->register_cmd(SLEEPCMD, &handle_sleep) + ctx->register_cmd(SEGCMD, &handle_segfault)
    + ctx->register_cmd(DIVZERO, &handle_divzero) + ctx->register_cmd(CALLBAD, (command_handler)0x01)
    + ctx->register_cmd(SPAMCMD, &handle_spam);
}

void module_cleanup() {
    ctx->unregister_cmd(SLEEPCMD);
    ctx->unregister_cmd(SEGCMD);
    ctx->unregister_cmd(DIVZERO);
    ctx->unregister_cmd(CALLBAD);
    ctx->unregister_cmd(SPAMCMD);
}
