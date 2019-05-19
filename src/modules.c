#include <stdio.h>
#include <stdlib.h>
#include <dlfcn.h>
#include <sys/types.h>
#include <sys/inotify.h>
#include <dirent.h>
#include <unistd.h>
#include <string.h>
#include <curl/curl.h>
#include "common.h"
#include "logger.h"
#include "module.h"
#include "modules.h"
#include "permissions.h"
#include "irc.h"
#include "threadpool.h"

/* command prefixes */
const char *cmdprefix;
int cmdprefix_len;

/* command registering */
struct command_tree {
    command_handler handler;
    struct command_tree *children[26];
};
struct command_tree cmds;
pthread_mutex_t modules_cmds_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t modules_array_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_t modules_watchdog_thread;

/* module loading */
const char *mod_dir;
int mod_dir_len;

int modules_register_cmd(const char *cmdname, command_handler handler) {
    struct command_tree *ptr = &cmds;
    char real;

    pthread_mutex_lock(&modules_cmds_mutex);
    logger_log(INFO, "commands", "registering command `%s`", cmdname);

    /* walk the tree! */
    while (ptr) {
        real = *(cmdname++);

        if (real == 0) {
            /* reached the end! */
            ptr->handler = handler;
            pthread_mutex_unlock(&modules_cmds_mutex);
            return 0;
        } else if (real >= 'A' && real <= 'Z') {
            /* convert to lower case */
            real += 32;
        } else if (real < 'a' || real > 'z') {
            /* invalid character */
            pthread_mutex_unlock(&modules_cmds_mutex);
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
    pthread_mutex_unlock(&modules_cmds_mutex);
    return -2;
}

int modules_unregister_cmd(const char *cmdname) {
    struct command_tree *ptr = &cmds;
    char real;

    pthread_mutex_lock(&modules_cmds_mutex);

    /* walk the tree! */
    while (ptr) {
        real = *(cmdname++);

        if (real == 0) {
            /* reached the end! */
            ptr->handler = NULL;
            pthread_mutex_unlock(&modules_cmds_mutex);
            return 0;
        } else if (real >= 'A' && real <= 'Z') {
            /* convert to lower case */
            real += 32;
        } else if (real < 'a' || real > 'z') {
            /* invalid character */
            pthread_mutex_unlock(&modules_cmds_mutex);
            return -1;
        }

        real -= 'a';

        /* loop */
        ptr = ptr->children[(int)real];
    }

    /* command not registered */
    pthread_mutex_unlock(&modules_cmds_mutex);
    return -3;
}

void modules_cmds_clean(struct command_tree *root, int depth) {
    int i;

    if (depth == 0) pthread_mutex_lock(&modules_cmds_mutex);

    /* recursively clean */
    for (i = 0; i < 26; i++) {
        if (root->children[i] != NULL) {
            modules_cmds_clean(root->children[i], depth+1);
            free(root->children[i]);
            root->children[i] = NULL;
        }
    }

    if (depth == 0) pthread_mutex_unlock(&modules_cmds_mutex);
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
    pthread_mutex_lock(&modules_cmds_mutex);
    while (ptr) {
        real = *(message++);

        if (real == ' ' || real == '\0') {
            /* reached the end, check if there is a command here */
            if (ptr->handler != NULL) {
                /* remove the command name */
                *(message - 1) = '\0';
                if (real == '\0') message--;

                /* run command in threadpool */
                logger_log(INFO, "commands", "Running command %s", message_orig);
                threadpool_queue_cmd(ptr->handler, message_orig, from, where, message);
            } else {
                break;
            }
            pthread_mutex_unlock(&modules_cmds_mutex);
            return;
        } else if (real >= 'A' && real <= 'Z') {
            /* convert to lower case */
            real += 32;
        } else if (real < 'a' || real > 'z') {
            /* invalid character */
            pthread_mutex_unlock(&modules_cmds_mutex);
            return;
        }

        real -= 'a';

        /* loop */
        ptr = ptr->children[(int)real];
    }

    /* command not found */
    pthread_mutex_unlock(&modules_cmds_mutex);
}

/* struct creation */
struct core_ctx *modules_ctx_new(void *dlsym, const char *fn) {
    int fn_l;
    struct core_ctx *ctx;

    /* allocate enough space for the string and the ctx */
    fn_l = strlen(fn) + 1;
    ctx = calloc(1, sizeof(struct core_ctx) + fn_l);
    if (!ctx) return ctx;

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
    ctx->fn = (char *)(ctx + 1);
    memcpy(ctx->fn, fn, fn_l);

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
    char local_file[PATH_MAX];
    int len, i;

    /* prepend `./` to so */
    len = strlen(mod_file);
    if (len + mod_dir_len + 1 > PATH_MAX) {
        logger_log(WARN, "commands", "failed to load module %s: filename too long", mod_file);
    }
    memcpy(local_file, mod_dir, mod_dir_len);
    memcpy(local_file + mod_dir_len, mod_file, len + 1);

    /* dlopen() the file */
    dlerror();
    handle = dlopen(local_file, RTLD_LAZY|RTLD_GLOBAL);
    if (!handle) {
        logger_log(WARN, "commands", "failed to load module %s: %s", mod_file, dlerror());
        return;
    }
    dlerror();

    /* get the init handler */
    *(void **) (&inith) = dlsym(handle, "module_init");
    if ((error = dlerror()) != NULL)  {
        logger_log(WARN, "commands", "failed to load module %s: %s", mod_file, error);
        dlclose(handle);
        return;
    }

    /* lock the mutex */
    pthread_mutex_lock(&modules_array_mutex);

    if (loaded_use + 1 >= loaded_len) {
        /* need to expand size of array */
        void **tmp;
        tmp = realloc(loaded, loaded_len * 2 * sizeof(void *));
        if (tmp == NULL) {
            logger_log(WARN, "commands", "system out of memory; failed to load module %s", mod_file);
            dlclose(handle);
            pthread_mutex_unlock(&modules_array_mutex);
            return;
        }
        loaded = tmp;
        memset(loaded + loaded_len, 0, loaded_len * sizeof(void *));
        loaded_len *= 2;
    }

    /* find space for the new context */
    for (i = 0; i < loaded_len; i++)
        if (!loaded[i]) break;

    if (i == loaded_len) __code_error("somehow the loaded_use is not reflective of how many modules are actually loaded");

    /* initialize new context */
    loaded[i] = modules_ctx_new(handle, mod_file);
    if (!loaded[i]) {
        logger_log(WARN, "commands", "system out of memory; failed to load module %s", mod_file);
        dlclose(handle);
        pthread_mutex_unlock(&modules_array_mutex);
        return;
    }

    /* queue the init call */
    threadpool_queue_init(loaded[i], inith, loaded + i);

    /* increment the number of used modules */
    loaded_use++;

    /* unlock the mutex */
    pthread_mutex_unlock(&modules_array_mutex);
}

void modules_unloadmod(struct core_ctx *ctx, void **ctx_pos, int wait) {
    module_cleanup_handler cleanuph;
    char *error;

    *(void **) (&cleanuph) = dlsym(ctx->dlsym, "module_cleanup");
    if ((error = dlerror()) != NULL)  {
        logger_log(WARN, "commands", "failed clean up module: %s", error);
        dlclose(ctx->dlsym);
        free(ctx);
        return;
    }

    threadpool_queue_deinit(cleanuph, ctx, ctx_pos, wait);
    loaded_use--;
}

void modules_rescanall() {
    DIR *folder;
    struct dirent *dir;
    const char *c;

    /* check that the modules have not already been loaded */
    if (loaded_use > 0) modules_unloadall();

    /* open folder */
    folder = opendir(mod_dir);
    if (folder == NULL) { perror("error: failed to open module directory"); exit(1); }

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

    /* unlock the mutex */
    pthread_mutex_unlock(&modules_array_mutex);
}

void modules_unloadall() {
    int i;

    logger_log(INFO, "commands", "unloading all modules...");

    /* lock the mutex */
    pthread_mutex_lock(&modules_array_mutex);

    /* wait for all the running threads to terminate */
    threadpool_waitall();

    /* clean up module lists */
    for (i = 0; i < loaded_len; i++) {
        if (loaded[i])
            modules_unloadmod((struct core_ctx *)loaded[i], loaded + i, 0);
    }

    /* clean the tree */
    modules_cmds_clean(&cmds, 0);
    logger_log(INFO, "commands", "unloaded all modules!");

    /* unlock the mutex */
    pthread_mutex_unlock(&modules_array_mutex);
}

struct modules_watchdir_wdfd {
    int wd, fd;
} wdfd;

void modules_watchdir_cleanup(void *arg) {
    inotify_rm_watch(wdfd.fd, wdfd.wd);
    close(wdfd.fd);
}

void *modules_watchdir(void *arg) {
    int r, i;
    /* buffer can store 1 inotify event */
    char in_buf[(sizeof(struct inotify_event) + NAME_MAX + 1)];
    struct inotify_event *evt = (struct inotify_event *)&in_buf;

    /* set up inotify */
    if ((wdfd.fd = inotify_init()) == -1) {
        /* error */
        logger_log(WARN, "commands", "Error setting up automatic module reloading");
        return NULL;
    }

    /* watch the modules dir for fully completed writes and deletions */
    if ((wdfd.wd = inotify_add_watch(wdfd.fd, mod_dir, IN_CLOSE_WRITE|IN_DELETE)) == -1) {
        /* error */
        logger_log(WARN, "commands", "Error setting up automatic module reloading");
        return NULL;
    }

    /* close the fd in case of errors */
    pthread_cleanup_push(modules_watchdir_cleanup, NULL);

    /* wait for events */
    while (1) {
        /* read inotify event */
        r = read(wdfd.fd, in_buf, sizeof(in_buf));
        if (r <= 0) {
            /* some sort of error occurred */
            logger_log(ERROR, "commands", "Automatic module reloading disabled due to error with inotify");
            break;
        }

        /* reload modules */
        logger_log(INFO, "commands", "%s module %s...", (evt->mask & IN_DELETE) ? "Unloading" : "Reloading", evt->name);

        /* lock the mutex because we are iterating through the array */
        pthread_mutex_lock(&modules_array_mutex);

        /* search for the module file */
        for (i = 0; i < loaded_len; i++) {
            if (loaded[i] && !strcmp(((struct core_ctx *)loaded[i])->fn, evt->name)) {
                /* found a match, unload this module */
                modules_unloadmod((struct core_ctx *)loaded[i], loaded + i, 1);
                break;
            }
        }

        /* done with the mutex now */
        pthread_mutex_unlock(&modules_array_mutex);

        /* if the event was a deletion, we're done now */
        if (evt->mask & IN_DELETE) continue;

        /* if not, we need to re-load the specific module */
        modules_loadmod(evt->name);
    }

    /* close the fd and clean up */
    pthread_cleanup_pop(1);

    return NULL;
}

void modules_deinit() {
    pthread_cancel(modules_watchdog_thread);
    pthread_join(modules_watchdog_thread, NULL);
    modules_unloadall();
    threadpool_deinit();
    curl_global_cleanup();
}

int modules_init() {
    mod_dir = getenv("BOT_MODULES_DIR");
    if (!mod_dir) mod_dir = "modules/";
    mod_dir_len = strlen(mod_dir);
    if (mod_dir[mod_dir_len-1] != '/') {
        logger_log(ERROR, "commands", "please ensure that the modules directory ends with a /");
    }

    /* initialize loaded variable */
    loaded = (void **)malloc(loaded_len * sizeof(void *));
    if (!loaded) {
        logger_log(ERROR, "commands", "OOM error when initializing");
        return -1;
    }
    memset(loaded, 0, loaded_len * sizeof(void *));

    /* get prefix length */
    if (cmdprefix == NULL) {
        logger_log(WARN, "commands", "command prefix not set, defaulting to ,");
        cmdprefix = ",";
    }
    cmdprefix_len = strlen(cmdprefix);

    /* initialize threadpool */
    threadpool_init();

    /* initialize CURL */
    curl_global_init(CURL_GLOBAL_ALL);

    /* initialize thread to watch directory */
    pthread_create(&modules_watchdog_thread, NULL, modules_watchdir, NULL);

    return 0;
}
