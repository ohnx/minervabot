#ifndef __THREADPOOL_H
#define __THREADPOOL_H
#include <pthread.h>
#include "module.h"

struct threadpool_args {
    command_handler handler;
    struct command_sender sender;
    size_t n;
    char *cmdname;
    char *from;
    char *where;
    char *args;
};

void threadpool_queue(command_handler handler, char *cmdname, char *from, char *where, char *args);
void threadpool_deinit();
int threadpool_init();

#endif /* __THREADPOOL_H */
