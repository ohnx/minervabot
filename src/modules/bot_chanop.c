#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "module.h"

struct core_ctx *ctx;

#define KICKCMD "kick"
#define VOICECMD "voice"

int handle_cmd(const char *cmdname, char *where, char *who, char *args) {
    int len;
    int user_perm_level;
    char *p = strchr(who, '!');
    char *h = strchr(who, '@') + 1;

    *p = '\0';

    user_perm_level = ctx->getperms(h);
    ctx->log(INFO, "perms", "user `%s` has perms %d", h, user_perm_level);

    if (user_perm_level < PERMS_HOP) {
        ctx->msgva(where, "%s: Insufficient permissions", who);
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
