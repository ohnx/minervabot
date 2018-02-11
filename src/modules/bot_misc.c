#include <stdio.h>
#include <string.h>
#include "module.h"

struct core_ctx *ctx;

#define HELPCMD "help"
#define HELLOCMD "hello"

int handle_help(const char *cmdname, struct command_sender who, char *where, char *args) {
    ctx->msgva(where, "%s: Hiya! I'm a bot written by ohnx. I'm open source! Please visit https://github.com/ohnx/minervabot :)", who.nick);
    return 0;
}

int handle_hello(const char *cmdname, struct command_sender who, char *where, char *args) {
    ctx->log(INFO, "hello", "[cmdname: %s] [where: %s] [who: %s!%s@%s] [args: %s]\n", cmdname, where, who.nick, who.ident, who.host, args);
    ctx->msg(where, "hello world!");
    return 0;
}

int module_init(struct core_ctx *core) {
    ctx = core;
    return ctx->register_cmd(HELPCMD, &handle_help) + ctx->register_cmd(HELLOCMD, &handle_hello);
}

void module_cleanup() {
    ctx->unregister_cmd(HELPCMD);
    ctx->unregister_cmd(HELLOCMD);
}
