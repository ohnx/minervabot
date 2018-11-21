#ifndef __THREADPOOL_H
#define __THREADPOOL_H
#include <pthread.h>
#include "module.h"

struct threadpool_cmd_args {
    size_t n;
    command_handler handler;
    struct command_sender sender;
    char *cmdname;
    char *from;
    char *where;
    char *args;
};

struct threadpool_init_args {
    size_t n;
    const char *modname;
    module_init_handler handler;
    struct core_ctx *ctx;
    void **ctx_pos;
};

struct threadpool_deinit_args {
    size_t n;
    module_cleanup_handler handler;
    struct core_ctx *ctx;
};

void threadpool_queue_cmd(command_handler handler, char *cmdname, char *from, char *where, char *args);
void threadpool_queue_init(const char *modname, module_init_handler handler, struct core_ctx *ctx, void **ctx_pos);
void threadpool_queue_deinit(module_cleanup_handler handler, struct core_ctx *ctx);
void threadpool_deinit();
int threadpool_init();

#endif /* __THREADPOOL_H */
