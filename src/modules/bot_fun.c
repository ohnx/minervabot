#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "module.h"

static struct core_ctx *ctx;

#define COOKIECMD "cookie"
#define HUGCMD "hug"
#define SHRUGCMD "shrug"
#define POTATOCMD "potato"
#define CELEBRATIONCMD "celebrate"
#define CONFETTICMD "confetti"
#define PARTYCMD "party"
#define FWCMD "fw"

static const char *cookietypes[8] = {"chocolate chip", "oatmeal raisin", "oatmeal chocolate", "oreo", "white chocolate macadamia nut", "peanut butter", "sugar", "ginger"};

static int handle_cookie(const char *cmdname, struct command_sender who, char *where, char *args) {
    if (*args == 0) args = (char *)who.nick;
    int r = rand() % (sizeof(cookietypes)/sizeof(const char *));

    ctx->msgva(where, "\001ACTION gives %s a %s cookie \xf0\x9f\x8d\xaa\001", args, cookietypes[r]);
    return 0;
}

static int handle_hug(const char *cmdname, struct command_sender who, char *where, char *args) {
    if (*args == 0) args = (char *)who.nick;
    ctx->msgva(where, "\001ACTION hugs %s\001", args);
    return 0;
}

static int handle_celebrate(const char *cmdname, struct command_sender who, char *where, char *args) {
    ctx->msgva(where, "\xf0\x9f\x8e\x89");
    return 0;
}

static int handle_shrug(const char *cmdname, struct command_sender who, char *where, char *args) {
    ctx->msgva(where, "\xc2\xaf\x5c\x5f\x28\xe3\x83\x84\x29\x5f\x2f\xc2\xaf");
    return 0;
}

static int handle_potato(const char *cmdname, struct command_sender who, char *where, char *args) {
    ctx->msg(where, "\xf0\x9f\xa5\x94");
    return 0;
}

static int handle_fw(const char *cmdname, struct command_sender who, char *where, char *args) {
    int i, n, q;
    unsigned char buf[511];
    unsigned short c, s;

    n = strlen(args);
    if (n > 170) n = 170;

    for (i = q = 0; i < n; i++) {
        c = args[i];
        if (c < 0x20 || c > 0x7F) continue;
        if (c == 0x20) {
            buf[q*3] = 0xe3;
            buf[q*3+1] = 0x80;
            buf[q*3+2] = 0x80;
        } else {
            /* idk why, but unicode has some weird stuff at U+FF40 */
            if (c >= '`') c += 0xC0;
            s = 0xBC60 + c;
            buf[q*3] = 0xef;
            buf[q*3+1] = (s >> 8) & 0xff;
            buf[q*3+2] = s & 0xff;
        }
        q++;
    }

    /* null-terminate */
    buf[q*3] = '\0';

    ctx->msg(where, (char *)buf);
    return 0;
}

int module_init(struct core_ctx *core) {
    ctx = core;
    srand(time(NULL));
    return ctx->register_cmd(COOKIECMD, &handle_cookie) + ctx->register_cmd(HUGCMD, &handle_hug)
         + ctx->register_cmd(SHRUGCMD, &handle_shrug) + ctx->register_cmd(POTATOCMD, &handle_potato)
         + ctx->register_cmd(CELEBRATIONCMD, &handle_celebrate) + ctx->register_cmd(CONFETTICMD, &handle_celebrate)
         + ctx->register_cmd(PARTYCMD, &handle_celebrate) + ctx->register_cmd(FWCMD, &handle_fw);
}

void module_cleanup() {
    ctx->unregister_cmd(COOKIECMD);
    ctx->unregister_cmd(HUGCMD);
    ctx->unregister_cmd(SHRUGCMD);
    ctx->unregister_cmd(POTATOCMD);
    ctx->unregister_cmd(CELEBRATIONCMD);
    ctx->unregister_cmd(CONFETTICMD);
    ctx->unregister_cmd(PARTYCMD);
    ctx->unregister_cmd(FWCMD);
}
