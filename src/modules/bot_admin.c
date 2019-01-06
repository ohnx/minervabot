#include <stdio.h>
#include <string.h>
#include <signal.h>
#include "module.h"

static struct core_ctx *ctx;

#define RELOADCMD "reload"
#define JOINCMD "join"
#define PARTCMD "part"
#define RAWCMD "raw"

static int handle_reload(const char *cmdname, struct command_sender who, char *where, char *args) {
    if (who.permission_level < PERMS_OWNER) {
        ctx->msgva(where, "%s: Insufficient permissions", who.nick);
        return 0;
    }

    ctx->msg(where, "Reloading modules");

    raise(SIGUSR1);

    return 0;
}

static int handle_cmd(const char *cmdname, struct command_sender who, char *where, char *args) {
    if (who.permission_level < PERMS_ADMIN) {
        ctx->msgva(where, "%s: Insufficient permissions", who.nick);
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

static int handle_raw(const char *cmdname, struct command_sender who, char *where, char *args) {
    if (who.permission_level < PERMS_OWNER) {
        ctx->msgva(where, "%s: Insufficient permissions", who.nick);
        return 0;
    }

    ctx->raw(args);
    return 0;
}

int module_init(struct core_ctx *core) {
    ctx = core;
    return ctx->register_cmd(RELOADCMD, &handle_reload) + ctx->register_cmd(JOINCMD, &handle_cmd) + ctx->register_cmd(PARTCMD, &handle_cmd) + ctx->register_cmd(RAWCMD, &handle_raw);
}

void module_cleanup() {
    ctx->unregister_cmd(RELOADCMD);
    ctx->unregister_cmd(JOINCMD);
    ctx->unregister_cmd(PARTCMD);
    ctx->unregister_cmd(RAWCMD);
}
