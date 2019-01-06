#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "module.h"

static struct core_ctx *ctx;

#define MODECMD "mode"
#define BANCMD "ban"
#define KICKCMD "kick"
#define OPCMD "op"
#define VOICECMD "voice"

static int handle_cmd(const char *cmdname, struct command_sender who, char *where, char *args) {
    int len;
    char cmd_buf[513];
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
        cmd_buf[0] = '+';
        cmd_buf[1] = 'v';
        cmd_buf[2] = ' ';
        memcpy(cmd_buf + 3, args, len + 1);
        ctx->chmode(where, cmd_buf);
        break;
    }

    if (who.permission_level < PERMS_OP) {
        ctx->msgva(where, "%s: Insufficient permissions", who.nick);
        return 0;
    }

    switch (*cmdname) {
    case 'm':
        ctx->chmode(where, args);
        break;
    case 'b':
        len = strlen(args);
        cmd_buf[0] = '+';
        cmd_buf[1] = 'b';
        cmd_buf[2] = ' ';
        memcpy(cmd_buf + 3, args, len + 1);
        ctx->chmode(where, cmd_buf);
        ctx->raw_va("REMOVE %s %s :Banned.\r\n", where, args);
        break;
    case 'o':
        len = strlen(args);
        cmd_buf[0] = '+';
        cmd_buf[1] = 'o';
        cmd_buf[2] = ' ';
        memcpy(cmd_buf + 3, args, len + 1);
        ctx->chmode(where, cmd_buf);
        break;
    }

    return 0;
}

int module_init(struct core_ctx *core) {
    ctx = core;
    return ctx->register_cmd(MODECMD, &handle_cmd) + ctx->register_cmd(BANCMD, &handle_cmd) +
        ctx->register_cmd(KICKCMD, &handle_cmd) + ctx->register_cmd(OPCMD, &handle_cmd) +
        ctx->register_cmd(VOICECMD, &handle_cmd);
}

void module_cleanup() {
    ctx->unregister_cmd(MODECMD);
    ctx->unregister_cmd(BANCMD);
    ctx->unregister_cmd(KICKCMD);
    ctx->unregister_cmd(OPCMD);
    ctx->unregister_cmd(VOICECMD);
}
