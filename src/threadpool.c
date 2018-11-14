#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <string.h>
#include "irc.h"
#include "permissions.h"
#include "threadpool.h"

struct threadpool_wrapper {
    struct threadpool_args *args;
    pthread_t thread;
};

unsigned char pool_flags;
struct threadpool_wrapper *pool = NULL;
pthread_attr_t attrs;
size_t pool_len = 2;

void *threadpool_wrapper(void *varg) {
    /* everything here is running in a thread */
    struct threadpool_args *arg = (struct threadpool_args *)varg;

    /* cleanup work */
    pthread_cleanup_push(free, varg);

    /* Get command sender info */
    irc_parsesender(&arg->sender, arg->from);

    /* check permissions */
    arg->sender.permission_level = permissions_get(arg->sender.host);

    /* run command handler */
    arg->handler(arg->cmdname, arg->sender, arg->where, arg->args);

    /* clean stuff up */
    while (pool_flags & 0x1) {usleep(10000);}
    pool[arg->n].args = NULL;
    pthread_cleanup_pop(1);

    /* done */
    pthread_exit(NULL);
}

void threadpool_queue(command_handler handler, char *cmdname, char *from, char *where, char *args) {
    size_t n, a, b, c, d, e;
    void *tmp;

    /* find the first unused thread slot */
    for (n = 0; n < pool_len; n++)
        if (!(pool[n].args)) break;

    /* increase threadpool size if necessary */
    if (n >= pool_len) {
        /* invalidate pool state */
        pool_flags |= 0x1;
        pool_len *= 2;
        tmp = realloc(pool, sizeof(struct threadpool_wrapper) * pool_len);

        if (!tmp) {
            logger_log(WARN, "threadpool", "OOM when trying to run command");
            return;
        }
        pool = (struct threadpool_wrapper *)tmp;

        /* allow threads to modify pool state again */
        pool_flags ^= 0x1;

        /* zero the new bytes */
        memset(&pool[pool_len/2], 0, sizeof(struct threadpool_wrapper)*pool_len/2);
    }

    /* allocate state+all data needed */
    a = sizeof(struct threadpool_args);
    b =  strlen(cmdname) + 1;
    c =  strlen(from) + 1;
    d =  strlen(where) + 1;
    e =  strlen(args) + 1;
    pool[n].args = malloc(a + b + c + d + e);
    if (!pool[n].args) {
        logger_log(WARN, "threadpool", "OOM when trying to run command");
        return;
    }

    /* set correct pointers */
    pool[n].args->handler = handler;
    pool[n].args->n = n;
    pool[n].args->cmdname = ((char *)pool[n].args)+a;
    pool[n].args->from = ((char *)pool[n].args)+a+b;
    pool[n].args->where = ((char *)pool[n].args)+a+b+c;
    pool[n].args->args = ((char *)pool[n].args)+a+b+c+d;

    /* set correct data */
    memcpy(pool[n].args->cmdname, cmdname, b);
    memcpy(pool[n].args->from, from, c);
    memcpy(pool[n].args->where, where, d);
    memcpy(pool[n].args->args, args, e);

    //hexDump ("my_str", &my_str, sizeof (my_str));

    /* create thread in pool */
    if (pthread_create(&(pool[n].thread), &attrs, threadpool_wrapper, pool[n].args)) {
        logger_log(WARN, "threadpool", "failed to create new thread to run command");
        return;
    }
}

void threadpool_deinit() {
    size_t n;

    pool_flags |= 0x1;
    for (n = 0; n < pool_len; n++) {
        if (pool[n].args) {
            pthread_cancel(pool[n].thread);
            pool[n].args = NULL;
        }
    }
    pool_flags ^= 0x1;

    free(pool);
    pthread_attr_destroy(&attrs);
    pool_len = 8;
}

int threadpool_init() {
    if (!pool)
        pool = malloc(sizeof(struct threadpool_wrapper) * pool_len);

    if (!pool) {
        logger_log(ERROR, "threadpool", "OOM error when initializing");
        return -1;
    }

    /* initialize memory */
    memset(pool, 0, sizeof(struct threadpool_wrapper)*pool_len);

    /* initialize pthreads */
    pthread_attr_init(&attrs);
    pthread_attr_setdetachstate(&attrs, PTHREAD_CREATE_DETACHED);

    return 0;
}
