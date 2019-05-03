#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <string.h>
#include <setjmp.h>
#include <signal.h>
#include <dlfcn.h>
#include <execinfo.h>
#include "irc.h"
#include "permissions.h"
#include "threadpool.h"

#define MAX_STACK_LEVELS 24

struct threadpool_wrapper {
    void *args;
    pthread_t thread;
};

unsigned char pool_flags;
struct threadpool_wrapper *pool = NULL;
pthread_attr_t attrs;
size_t pool_len = 2;
pthread_key_t sigsegv_jmp_point;
int print_st;

static void sigsegv_handler(int sig, siginfo_t *siginfo, void *arg) {
    if (print_st) {
        /* this is technically undefined behaviour */
        void *buffer[MAX_STACK_LEVELS];
        int levels = backtrace(buffer, MAX_STACK_LEVELS);

        backtrace_symbols_fd(buffer + 1, levels - 1, 2);
    }
    /* longjmp to the thread-specific point */
   longjmp(*((sigjmp_buf *)pthread_getspecific(sigsegv_jmp_point)), 1);
}

void threadpool_catcherrors() {
    struct sigaction sa;

    /* catch thread-specific errors */
    memset(&sa, 0, sizeof(struct sigaction));
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_NODEFER;
    sa.sa_sigaction = sigsegv_handler;
    sigaction(SIGBUS, &sa, NULL);
    sigaction(SIGFPE, &sa, NULL);
    sigaction(SIGILL, &sa, NULL);
    sigaction(SIGSEGV, &sa, NULL);
}

void *threadpool_wrapper_cmd(void *varg) {
    /* everything here is running in a thread */
    sigjmp_buf point;
    struct threadpool_cmd_args *arg = (struct threadpool_cmd_args *)varg;

    /* catch error if setjmp() returns 1 */
    if (setjmp(point) == 1) {
        /* an error occurred */
        logger_log(WARN, "commands", "an error occurred while trying to call command %s", arg->cmdname);
        goto execution_done;
    }

    /* catch errors in this thread */
    pthread_setspecific(sigsegv_jmp_point, &point);
    threadpool_catcherrors();

    /* cleanup work */
    pthread_cleanup_push(free, varg);

    /* Get command sender info */
    irc_parsesender(&arg->sender, arg->from);

    /* check permissions */
    arg->sender.permission_level = permissions_get(arg->sender.host);

    /* run command handler */
    arg->handler(arg->cmdname, arg->sender, arg->where, arg->args);

execution_done:
    /* clean stuff up */
    while (pool_flags & 0x1) {usleep(10000);}
    pool[arg->n].args = NULL;
    pthread_cleanup_pop(1);

    /* done */
    pthread_exit(NULL);
}

void *threadpool_wrapper_init(void *varg) {
    /* everything here is running in a thread */
    sigjmp_buf point;
    struct threadpool_init_args *arg = (struct threadpool_init_args *)varg;
    int ret;

    /* catch error if setjmp() returns 1 */
    if (setjmp(point) == 1) goto execution_failed;

    /* catch errors in this thread */
    pthread_setspecific(sigsegv_jmp_point, &point);
    threadpool_catcherrors();

    /* cleanup work */
    pthread_cleanup_push(free, varg);

    /* try to run the init handler */
    ret = arg->handler(arg->ctx);
    if (!ret) goto execution_done;

execution_failed:
    /* execution failed, close handler and stuff */
    logger_log(WARN, "commands", "an error occurred while trying to call initialize module %s", arg->modname);
    *(arg->ctx_pos) = NULL;
    dlclose((arg->ctx)->dlsym);
    free(arg->ctx);

execution_done:
    /* clean stuff up */
    while (pool_flags & 0x1) {usleep(10000);}
    pool[arg->n].args = NULL;
    pthread_cleanup_pop(1);

    /* done */
    pthread_exit(NULL);
}

void *threadpool_wrapper_deinit(void *varg) {
    /* everything here is running in a thread */
    sigjmp_buf point;
    struct threadpool_deinit_args *arg = (struct threadpool_deinit_args *)varg;

    /* catch error if setjmp() returns 1 */
    if (setjmp(point) == 1) {
        logger_log(WARN, "commands", "an error occurred while trying to clean up module");
        goto execution_done;
    }

    /* catch errors in this thread */
    pthread_setspecific(sigsegv_jmp_point, &point);
    threadpool_catcherrors();

    /* cleanup work */
    pthread_cleanup_push(free, varg);

    /* try to run the init handler */
    arg->handler();

execution_done:
    /* clean stuff up */
    dlclose((arg->ctx)->dlsym);
    free(arg->ctx);
    while (pool_flags & 0x1) {usleep(10000);}
    pool[arg->n].args = NULL;
    pthread_cleanup_pop(1);

    /* done */
    pthread_exit(NULL);
}

int threadpool_queue_findspace(size_t *n) {
    /* find the first unused thread slot */
    for (*n = 0; *n < pool_len; (*n)++)
        if (!(pool[*n].args)) break;

    /* increase threadpool size if necessary */
    if (*n >= pool_len) {
        void *tmp;

        /* invalidate pool state */
        pool_flags |= 0x1;
        pool_len *= 2;
        tmp = realloc(pool, sizeof(struct threadpool_wrapper) * pool_len);

        if (!tmp) {
            logger_log(WARN, "threadpool", "OOM when trying to find space in threadpool");
            return -1;
        }
        pool = (struct threadpool_wrapper *)tmp;

        /* allow threads to modify pool state again */
        pool_flags ^= 0x1;

        /* zero the new bytes */
        memset(&pool[pool_len/2], 0, sizeof(struct threadpool_wrapper)*pool_len/2);
    }

    return 0;
}

void threadpool_queue_cmd(command_handler handler, char *cmdname, char *from, char *where, char *args) {
    size_t n, a, b, c, d, e;

    /* find space in queue to place this thread handler */
    if (threadpool_queue_findspace(&n)) return;

    /* allocate state+all data needed */
    a = sizeof(struct threadpool_cmd_args);
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
    ((struct threadpool_cmd_args *)pool[n].args)->n = n;
    ((struct threadpool_cmd_args *)pool[n].args)->handler = handler;
    ((struct threadpool_cmd_args *)pool[n].args)->cmdname = ((char *)pool[n].args)+a;
    ((struct threadpool_cmd_args *)pool[n].args)->from = ((char *)pool[n].args)+a+b;
    ((struct threadpool_cmd_args *)pool[n].args)->where = ((char *)pool[n].args)+a+b+c;
    ((struct threadpool_cmd_args *)pool[n].args)->args = ((char *)pool[n].args)+a+b+c+d;

    /* set correct data */
    memcpy(((struct threadpool_cmd_args *)pool[n].args)->cmdname, cmdname, b);
    memcpy(((struct threadpool_cmd_args *)pool[n].args)->from, from, c);
    memcpy(((struct threadpool_cmd_args *)pool[n].args)->where, where, d);
    memcpy(((struct threadpool_cmd_args *)pool[n].args)->args, args, e);

    /* create thread in pool */
    if (pthread_create(&(pool[n].thread), &attrs, threadpool_wrapper_cmd, pool[n].args)) {
        logger_log(WARN, "threadpool", "failed to create new thread to run command");
        return;
    }
}

void threadpool_queue_init(const char *modname, module_init_handler handler, struct core_ctx *ctx, void **ctx_pos) {
    size_t n;

    /* find space in queue to place this thread handler */
    if (threadpool_queue_findspace(&n)) return;

    /* allocate all data needed */
    pool[n].args = malloc(sizeof(struct threadpool_init_args));
    if (!pool[n].args) {
        logger_log(WARN, "threadpool", "OOM when trying to init module");
        return;
    }

    /* set correct data */
    ((struct threadpool_init_args *)pool[n].args)->n = n;
    ((struct threadpool_init_args *)pool[n].args)->modname = modname;
    ((struct threadpool_init_args *)pool[n].args)->handler = handler;
    ((struct threadpool_init_args *)pool[n].args)->ctx = ctx;
    ((struct threadpool_init_args *)pool[n].args)->ctx_pos = ctx_pos;

    /* create thread in pool */
    if (pthread_create(&(pool[n].thread), &attrs, threadpool_wrapper_init, pool[n].args)) {
        logger_log(WARN, "threadpool", "failed to create new thread to init module");
        return;
    }
}

void threadpool_queue_deinit(module_cleanup_handler handler, struct core_ctx *ctx) {
    size_t n;

    /* find space in queue to place this thread handler */
    if (threadpool_queue_findspace(&n)) return;

    /* allocate all data needed */
    pool[n].args = malloc(sizeof(struct threadpool_deinit_args));
    if (!pool[n].args) {
        logger_log(WARN, "threadpool", "OOM when trying to init module");
        return;
    }

    /* set correct data */
    ((struct threadpool_deinit_args *)pool[n].args)->n = n;
    ((struct threadpool_deinit_args *)pool[n].args)->handler = handler;
    ((struct threadpool_deinit_args *)pool[n].args)->ctx = ctx;

    /* create thread in pool */
    if (pthread_create(&(pool[n].thread), &attrs, threadpool_wrapper_deinit, pool[n].args)) {
        logger_log(WARN, "threadpool", "failed to create new thread to deinit module");
        return;
    }
}

void threadpool_deinit() {
    size_t n;

    pool_flags |= 0x1;
    for (n = 0; n < pool_len; n++) {
        if (pool[n].args) {
            pthread_join(pool[n].thread, NULL);
            pool[n].args = NULL;
        }
    }
    pool_flags ^= 0x1;

    free(pool);
    pthread_attr_destroy(&attrs);
    pthread_key_delete(sigsegv_jmp_point);
    pool_len = 8;
}

int threadpool_init() {
    char *p;

    if (!pool)
        pool = malloc(sizeof(struct threadpool_wrapper) * pool_len);

    if (!pool) {
        logger_log(ERROR, "threadpool", "OOM error when initializing");
        return -1;
    }

    /* check if print stacktrace */
    print_st = (p = getenv("BOT_ENABLE_DEBUG")) && (atoi(p) == 1);

    /* initialize memory */
    memset(pool, 0, sizeof(struct threadpool_wrapper)*pool_len);

    /* initialize pthreads */
    pthread_attr_init(&attrs);
    pthread_attr_setdetachstate(&attrs, PTHREAD_CREATE_DETACHED);
    pthread_key_create(&sigsegv_jmp_point, NULL);

    return 0;
}
