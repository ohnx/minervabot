#include "netabs.h"
static char sbuf[512];

void net_raw(bot_context *bc, const char *str) {
    printf("<< %s\n", str);
    write(bc->conn, str, strlen(str));
    write(bc->conn, "\r\n", 2);
}

void net_raw_va(bot_context *bc, const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(sbuf, 512, fmt, ap);
    va_end(ap);
    printf("<< %s\n", sbuf);
    write(bc->conn, sbuf, strlen(sbuf));
    write(bc->conn, "\r\n", 2);
}

void bot_initsetup(bot_context *bc) {
    net_raw(bc, PRECONNECT_COMMAND);
    net_raw(bc, "USER "BOT_USER"8 * :minervabot; athenabot reborn!");
    net_raw(bc, "NICK "DEFAULT_BOT_NICK);
}

bot_context *bot_connect(const char *host, const char *port) {
    struct addrinfo hints;
    bot_context *bc;
    
    bc = calloc(1, sizeof(bot_context));
    
    /* setup socket */
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    getaddrinfo(host, port, &hints, &(bc->res));
    
    /* connect */
    bc->conn = socket((bc->res)->ai_family, (bc->res)->ai_socktype, (bc->res)->ai_protocol);
    connect(bc->conn, (bc->res)->ai_addr, (bc->res)->ai_addrlen);
    
    /* initial connection info */
    bot_initsetup(bc);
    
    /* set nick to default nick */
    bc->nick = strdup(DEFAULT_BOT_NICK);
    
    return bc;
}

void bot_joinchan(bot_context *bc, const char *chan) {
    net_raw_va(bc, "JOIN %s", chan);
}

void bot_changenick(bot_context *bc, const char *nick) {
    free(bc->nick);
    bc->nick = strdup(nick);
    net_raw_va(bc, "NICK %s", nick);
}