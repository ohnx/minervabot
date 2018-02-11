#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "module.h"

struct core_ctx *ctx;

#define KICKCMD "kick"
#define VOICECMD "voice"

int handle_cmd(const char *cmdname, struct command_sender who, char *where, char *args) {
    int len;
    char *p;

    if (who.permission_level < PERMS_HOP) {
        ctx->msgva(where, "%s: Insufficient permissions", who.nick);
        return 0;
    }

    switch (*cmdname) {
    case 'k':
        p = strchr(args, ' ');
        if (p) {
            *p = '\0';
            ctx->kick(where, args, p+1);
        } else {
            ctx->kick(where, args, NULL);
        }
        break;
    case 'v':
        len = strlen(args);
        p = malloc(len + 4);
        p[0] = '+';
        p[1] = 'v';
        p[2] = ' ';
        memcpy(p+3, args, len+1);
        ctx->chmode(where, p);
        free(p);
        break;
    }
    return 0;
}

int module_init(struct core_ctx *core) {
    ctx = core;
    return ctx->register_cmd(KICKCMD, &handle_cmd) + ctx->register_cmd(VOICECMD, &handle_cmd);
}

void module_cleanup() {
    ctx->unregister_cmd(KICKCMD);
    ctx->unregister_cmd(VOICECMD);
}
