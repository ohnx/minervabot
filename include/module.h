#ifndef __MODULE_H
#define __MODULE_H
#include "logger.h"
#include "perms.h"

/* this file is used by modules */

/**
 * a handler for a command
 * 
 * @param cmdname the name of the command
 * @param where where the command was run
 * @param who who ran the command (the full nick+ident+hostmask)
 * @param args other arguments passed to the command
 * @return whether or not the command ran without an error
 */
typedef int (*command_handler)(const char *cmdname, char *where, char *who, char *args);

struct core_ctx {
    /* modules.h commands */
    /* register a command */
    int (*register_cmd)(const char *cmdname, command_handler handler);
    /* unregister a command */
    int (*unregister_cmd)(const char *cmdname);

    /* logger.h commands */
    /* log some info to the screen */
    void (*log)(enum logger_level level, const char *module, const char *fmt, ...);

    /* permissions.h commands */
    /* set permissions for a host to <level> */
    void (*setperms)(const char *host, int level);
    /* get permissions for a host */
    int (*getperms)(const char *host);

    /* irc.h commands */
    /* remove \r and \n from the string */
    void (*mksafe)(char *str);
    /* send a message to a channel */
    void (*msg)(const char *chan, const char *message);
    /* send a message to a channel using format string */
    void (*msgva)(const char *chan, const char *fmt, ...);
    /* send an action to a channel */
    void (*action)(const char *chan, const char *message);
    /* join a channel */
    void (*join)(const char *chan);
    /* part a channel */
    void (*part)(const char *chan, const char *message);
    /* change modes in a channel */
    void (*chmode)(const char *chan, const char *modes);
    /* kick a user from a channel (reason can be null) */
    void (*kick)(const char *chan, const char *nick, const char *reason);
    /* send raw message */
    void (*raw)(const char *message);
    /* send raw message (manually add \r\n) "variadically" */
    void (*raw_va)(const char *fmt, ...);

    /* internal stuff */
    /* handle from dlopen */
    void *dlsym;
};

/**
 * modules will have an init handler to register all commands, etc.
 * this function is given a context to the core. please keep it safe!
 */
typedef int (*module_init_handler)(struct core_ctx *);
/**
 * modules may have a cleanup handler to unregister commands, etc.
 */
typedef void (*module_cleanup_handler)();

#endif /* __MODULE_H */
