#ifndef __MODULE_H
#define __MODULE_H
typedef int (*command_handler)(const char *cmdname, char *where, char *who, char *args);

struct core_ctx {
    /* modules.h commands */
    int (*register_cmd)(const char *cmdname, command_handler handler);
    int (*unregister_cmd)(const char *cmdname);

    /* irc.h commands */
    void (*mksafe)(char *str);
    void (*msg)(const char *chan, const char *message);
    void (*action)(const char *chan, const char *message);
    void (*join)(const char *chan);
    void (*part)(const char *chan, const char *message);
    void (*chmode)(const char *chan, const char *modes);
    void (*kick)(const char *chan, const char *nick, const char *reason);
    void (*raw)(const char *message);

    /* internal stuff */
    void *dlsym;
};

typedef int (*module_init_handler)(struct core_ctx *);
typedef void (*module_cleanup_handler)();

#endif /* __MODULE_H */
