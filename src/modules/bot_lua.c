#include <unistd.h>
#include <stdio.h>
#include <dirent.h>
#include <string.h>
#include <sys/stat.h>
#include <stdlib.h>
#include "module.h"

#if 0
struct core_ctx *ctx;

int module_init(struct core_ctx *core) {
    DIR *dp;
    char *dot;
    struct dirent *entry;
    struct stat statbuf;

    ctx = core;

    /* open directory */
    if ((dp = opendir(".")) == NULL) {
        ctx->log(WARN, "lua", "failed to read lua modules directory");
        return -1;
    }

    /* loop through all entries in directory */
    while ((entry = readdir(dp)) != NULL) {
        /* check if this file ends with .lua */
        dot = strchr(entry->d_name, '.');
        if (!dot || strcmp(dot, "lua")) continue;

        /* check that this is a regular file*/
        lstat(entry->d_name, &statbuf);
        if (!S_ISREG(statbuf.st_mode) && !S_ISLNK(statbuf.st_mode)) continue;

        /* initialize new lua context for this file */

        /* open the file and parse it */
    }

    closedir(dp);


    srand(time(NULL));
    return ctx->register_cmd(COOKIECMD, &handle_cookie) + ctx->register_cmd(HUGCMD, &handle_hug)
         + ctx->register_cmd(SHRUGCMD, &handle_shrug) + ctx->register_cmd(POTATOCMD, &handle_potato)
         + ctx->register_cmd(CELEBRATIONCMD, &handle_celebrate) + ctx->register_cmd(CONFETTICMD, &handle_celebrate);
}

void module_cleanup() {
    ctx->unregister_cmd(COOKIECMD);
    ctx->unregister_cmd(HUGCMD);
    ctx->unregister_cmd(SHRUGCMD);
    ctx->unregister_cmd(POTATOCMD);
    ctx->unregister_cmd(CELEBRATIONCMD);
    ctx->unregister_cmd(CONFETTICMD);
}
#endif