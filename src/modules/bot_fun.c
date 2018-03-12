#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "module.h"

struct core_ctx *ctx;

#define COOKIECMD "cookie"
#define HUGCMD "hug"

const char *cookietypes[8] = {"chocolate chip", "oatmeal raisin", "oatmeal chocolate", "oreo", "white chocolate macadamia nut", "peanut butter", "sugar", "ginger"};

int handle_cookie(const char *cmdname, struct command_sender who, char *where, char *args) {
    if (*args == 0) args = (char *)who.nick;
    int r = rand() % (sizeof(cookietypes)/sizeof(const char *));

    ctx->msgva(where, "\001ACTION gives %s a %s cookie\001", args, cookietypes[r]);
    return 0;
}

int handle_hug(const char *cmdname, struct command_sender who, char *where, char *args) {
    if (*args == 0) args = (char *)who.nick;
    ctx->msgva(where, "\001ACTION hugs %s\001", args);
    return 0;
}

int module_init(struct core_ctx *core) {
    ctx = core;
    srand(time(NULL));
    return ctx->register_cmd(COOKIECMD, &handle_cookie) + ctx->register_cmd(HUGCMD, &handle_hug);
}

void module_cleanup() {
    ctx->unregister_cmd(COOKIECMD);
    ctx->unregister_cmd(HUGCMD);
}
