#include <stdio.h>
#include <stdlib.h>
#include <dlfcn.h>
#include <sys/types.h>
#include <dirent.h>
#include <unistd.h>
#include <string.h>
#include "logger.h"
#include "module.h"
#include "modules.h"
#include "permissions.h"
#include "irc.h"

/* command prefixes */
const char *cmdprefix;
int cmdprefix_len;

/* command registering */
struct command_tree {
    command_handler handler;
    struct command_tree *children[26];
};

struct command_tree cmds;

const char *mod_dir;

int modules_register_cmd(const char *cmdname, command_handler handler) {
    struct command_tree *ptr = &cmds;
    char real;

    logger_log(INFO, "commands", "registering command `%s`", cmdname);

    /* walk the tree! */
    while (ptr) {
        real = *(cmdname++);

        if (real == 0) {
            /* reached the end! */
            ptr->handler = handler;
            return 0;
        } else if (real >= 'A' && real <= 'Z') {
            /* convert to lower case */
            real += 32;
        } else if (real < 'a' || real > 'z') {
            /* invalid character */
            return -1;
        }

        real -= 'a';

        /* allocate memory for new node */
        if (ptr->children[(int)real] == NULL)
            ptr->children[(int)real] = calloc(1, sizeof(struct command_tree));

        /* loop */
        ptr = ptr->children[(int)real];
    }

    /* OOM or some other tree walking error */
    return -2;
}

int modules_unregister_cmd(const char *cmdname) {
    struct command_tree *ptr = &cmds;
    char real;

    /* walk the tree! */
    while (ptr) {
        real = *(cmdname++);

        if (real == 0) {
            /* reached the end! */
            ptr->handler = NULL;
            return 0;
        } else if (real >= 'A' && real <= 'Z') {
            /* convert to lower case */
            real += 32;
        } else if (real < 'a' || real > 'z') {
            /* invalid character */
            return -1;
        }

        real -= 'a';

        /* loop */
        ptr = ptr->children[(int)real];
    }

    /* command not registered */
    return -3;
}

void modules_cmds_clean(struct command_tree *root) {
    int i;
    /* recursively clean */
    for (i = 0; i < 26; i++) {
        if (root->children[i] != NULL) {
            modules_cmds_clean(root->children[i]);
            free(root->children[i]);
            root->children[i] = NULL;
        }
    }
}

/* command invoking */
void modules_check_cmd(char *from, char *where, char *message) {
    int i = 0, message_len;
    struct command_tree *ptr = &cmds;
    char real;
    char *message_orig;

    /* ensure we don't overflow */
    message_len = strlen(message);
    if (cmdprefix_len + 1 > message_len) return;

    /* check command prefix first */
    if (strncmp(cmdprefix, message, cmdprefix_len)) return;

    /* skip past command prefix */
    message += i + 1;
    message_orig = message;

    /* time to walk the tree */
    while (ptr) {
        real = *(message++);

        if (real == ' ' || real == '\0') {
            /* reached the end, check if there is a command here */
            if (ptr->handler != NULL) {
                struct command_sender sender;

                /* Remove the command name */
                *(message - 1) = '\0';
                if (real == '\0') message--;

                /* Get command sender info */
                irc_parsesender(&sender, from);
                sender.permission_level = permissions_get(sender.host);

                logger_log(INFO, "commands", "Running command %s", message_orig);
                ptr->handler(message_orig, sender, where, message);
            } else {
                break;
            }
            return;
        } else if (real >= 'A' && real <= 'Z') {
            /* convert to lower case */
            real += 32;
        } else if (real < 'a' || real > 'z') {
            /* invalid character */
            return;
        }

        real -= 'a';

        /* loop */
        ptr = ptr->children[(int)real];
    }

    /* command not found */
}

/* struct creation */
struct core_ctx *modules_ctx_new(void *dlsym) {
    struct core_ctx *ctx = calloc(1, sizeof(struct core_ctx));

    /* modules.h commands */
    ctx->register_cmd = &modules_register_cmd;
    ctx->unregister_cmd = &modules_unregister_cmd;

    /* logger.h commands */
    ctx->log = &logger_log;

    /* permissions.h commands */
    ctx->setperms = &permissions_set;
    ctx->getperms = &permissions_get;

    /* irc.h commands */
    ctx->mksafe = &irc_filter;
    ctx->msg = &irc_message;
    ctx->msgva = &irc_message_va;
    ctx->action = &irc_action;
    ctx->join = &irc_join;
    ctx->part = &irc_part;
    ctx->chmode = &irc_chmode;
    ctx->kick = &irc_kick;
    ctx->raw = &irc_raw;
    ctx->raw_va = &net_raw;

    /* internal stuff */
    ctx->dlsym = dlsym;

    return ctx;
}

/* loaded module list */
int loaded_len = 6;
int loaded_use = 0;
void **loaded;

void modules_loadmod(const char *mod_file) {
    void *handle;
    module_init_handler inith;
    char *error;
    char *local_file;
    int len;

    /* prepend `./` to so */
    len = strlen(mod_file);
    local_file = malloc(len + 3);
    local_file[0] = '.';
    local_file[1] = '/';
    memcpy(local_file+2, mod_file, len+1);

    dlerror();
    handle = dlopen(local_file, RTLD_LAZY);
    free(local_file);
    if (!handle) {
        logger_log(WARN, "commands", "failed to load module %s: %s", mod_file, dlerror());
        return;
    }
    dlerror();

    inith = dlsym(handle, "module_init");
    if ((error = dlerror()) != NULL)  {
        logger_log(WARN, "commands", "failed to load module %s: %s", mod_file, error);
        return;
    }

    if (loaded_use == loaded_len) {
        /* need to expand size of array */
        void **tmp;
        loaded_len *= 2;
        tmp = realloc(loaded, loaded_len * sizeof(void *));
        if (tmp == NULL) {
            logger_log(WARN, "commands", "system out of memory; failed to load module");
            dlclose(handle);
            return;
        }
    }

    loaded[loaded_use++] = modules_ctx_new(handle);

    if ((*inith)(loaded[loaded_use - 1])) {
        /* failed to init module, so close it */
        dlclose(handle);
        loaded[loaded_use--] = NULL;
    }
}

void modules_unloadmod(struct core_ctx *ctx) {
    module_cleanup_handler cleanuph;
    char *error;

    cleanuph = dlsym(ctx->dlsym, "module_cleanup");
    if ((error = dlerror()) != NULL)  {
        logger_log(WARN, "commands", "failed clean up module: %s", error);
        dlclose(ctx->dlsym);
        return;
    }

    (*cleanuph)();
    dlclose(ctx->dlsym);
}

/* public functions */
void modules_rescanall() {
    DIR *folder;
    struct dirent *dir;
    const char *c;

    /* cd to modules dir */
    if (chdir(mod_dir)) {
        logger_log(ERROR, "commands", "error: failed to change to modules directory");
        return;
    }

    /* check that the modules have not already been loaded */
    if (loaded_use > 0) modules_unloadall();

    /* open folder */
    folder = opendir(".");
    if (folder == NULL) { perror("error: failed to open directory"); exit(1); }

    /* loop through all files in folder */
    while ((dir = readdir(folder)) != NULL) {
        if (*(dir->d_name) == '.') continue;
        c = strrchr(dir->d_name, '.');
        if (c != NULL && *(++c) == 's' && *(++c) == 'o') {
            logger_log(INFO, "commands", "loading module `%s`", dir->d_name);
            modules_loadmod(dir->d_name);
        } else {
            logger_log(WARN, "commands", "non-module file `%s` in modules directory", dir->d_name);
        }
    }

    /* clean up */
    closedir(folder);
    chdir("../");
}

void modules_unloadall() {
    int i;

    logger_log(INFO, "commands", "unloading all modules...");

    /* clean up module lists */
    for (i = 0; i < loaded_use; i++) {
        modules_unloadmod((struct core_ctx *)loaded[i]);
        free(loaded[i]);
    }
    loaded_use = 0;
    loaded_len = 6;
    loaded = realloc(loaded, loaded_len * sizeof(void *));

    modules_cmds_clean(&cmds);
    logger_log(INFO, "commands", "unloaded all modules!");
}

void modules_deinit() {
    modules_unloadall();
    free(loaded);
}

void modules_init() {
    mod_dir = getenv("BOT_MODULES_DIR");
    if (!mod_dir) mod_dir = "modules/";

    /* initialize loaded variable */
    loaded = (void **)malloc(loaded_len * sizeof(void *));

    /* get prefix length */
    if (cmdprefix == NULL) {
        logger_log(WARN, "commands", "command prefix not set, defaulting to ,");
        cmdprefix = ",";
    }
    cmdprefix_len = strlen(cmdprefix);
}
