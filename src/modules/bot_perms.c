#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "module.h"
#include "perms.h"

struct core_ctx *ctx;

#define SETPERMS_CMD "setp"
#define GETPERMS_CMD "getp"

int handle_cmd(const char *cmdname, char *where, char *who, char *args) {
    int perm_level, user_perm_level;
    char *p = strchr(who, '!');
    char *h = strchr(who, '@') + 1;
    char *t;

    *p = '\0';

    user_perm_level = ctx->getperms(h);
    ctx->log(INFO, "perms", "user `%s` has perms %d", h, user_perm_level);

    switch (*cmdname) {
    case 's':
        t = strchr(args, ' ');
        if (t == NULL) {
            /* invalid usage */
            ctx->msgva(where, "%s: Invalid usage of command!", who);
            return 0;
        }
        *(t++) = '\0';
        /* 1st arg is permission, 2nd is hostmask to set */
        perm_level = atoi(args);
        if (user_perm_level >= PERMS_OP && perm_level < user_perm_level) {
            /* user is permitted to change this */
            ctx->setperms(t, perm_level);
            ctx->msgva(where, "%s: Set permission level for %s to %d!", who, t, perm_level);
        } else {
            ctx->msgva(where, "%s: You lack sufficient permission to give %s permission level %d.", who, t, perm_level);
        }
        break;
    case 'g':
        if (strlen(args) > 0 && user_perm_level >= PERMS_HOP) {
            perm_level = ctx->getperms(args);
            ctx->msgva(where, "%s: %s has permission level %d", who, args, perm_level);
        } else {
            ctx->msgva(where, "%s: you have permission level %d", who, user_perm_level);
        }
        break;
    }
    return 0;
}

int module_init(struct core_ctx *core) {
    ctx = core;
    return ctx->register_cmd(SETPERMS_CMD, &handle_cmd) + ctx->register_cmd(GETPERMS_CMD, &handle_cmd);
}

void module_cleanup() {
    ctx->unregister_cmd(SETPERMS_CMD);
    ctx->unregister_cmd(GETPERMS_CMD);
}
