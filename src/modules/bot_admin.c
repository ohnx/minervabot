#include <stdio.h>
#include <string.h>
#include "module.h"

struct core_ctx *ctx;

#define RAWCMD "raw"
#define JOINCMD "join"
#define PARTCMD "part"

int handle_cmd(const char *cmdname, char *where, char *who, char *args) {
    int user_perm_level;
    char *p = strchr(who, '!');
    char *h = strchr(who, '@') + 1;

    *p = '\0';

    user_perm_level = ctx->getperms(h);
    ctx->log(INFO, "perms", "user `%s` has perms %d", h, user_perm_level);

    if (user_perm_level < PERMS_ADMIN) {
        ctx->msgva(where, "%s: Insufficient permissions", who);
        return 0;
    }

    switch (*cmdname) {
    case 'j':
        ctx->join(args);
        break;
    case 'p':
        ctx->part(args, "You're gonna miss me when I'm gone!");
        break;
    }
    return 0;
}

int handle_raw(const char *cmdname, char *where, char *who, char *args) {
    int user_perm_level;
    char *p = strchr(who, '!');
    char *h = strchr(who, '@') + 1;

    *p = '\0';

    user_perm_level = ctx->getperms(h);
    ctx->log(INFO, "perms", "user `%s` has perms %d", h, user_perm_level);

    if (user_perm_level < PERMS_OWNER) {
        ctx->msgva(where, "%s: Insufficient permissions", who);
        return 0;
    }

    ctx->raw(args);
    return 0;
}

int module_init(struct core_ctx *core) {
    ctx = core;
    return ctx->register_cmd(JOINCMD, &handle_cmd) + ctx->register_cmd(PARTCMD, &handle_cmd) + ctx->register_cmd(RAWCMD, &handle_raw);
}

void module_cleanup() {
    ctx->unregister_cmd(JOINCMD);
    ctx->unregister_cmd(PARTCMD);
    ctx->unregister_cmd(RAWCMD);
}
