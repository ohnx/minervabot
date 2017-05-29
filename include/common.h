#ifndef __MINERVA_COMMON_INC
#define __MINERVA_COMMON_INC

typedef struct _bot_context {
    int conn;
    struct addrinfo *res;
    char *nick;
} bot_context;

#endif
