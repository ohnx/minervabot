#include <stdio.h>
#include <stdarg.h>
#include <unistd.h>
#include <string.h>
#include <netdb.h>
#include <errno.h>
#include "net.h"

int conn;
char sbuf[512];

void net_raw(const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(sbuf, 512, fmt, ap);
    va_end(ap);
    printf("<< %s", sbuf);
    write(conn, sbuf, strlen(sbuf));
}

void net_raws(char *ptr) {
    printf("<< %s", ptr);
    write(conn, ptr, strlen(ptr));
}

void net_connect(const char *host, const char *port) {
    struct addrinfo hints, *res;

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    getaddrinfo(host, port, &hints, &res);

    conn = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    connect(conn, res->ai_addr, res->ai_addrlen);

    freeaddrinfo(res);
}

void net_disconnect() {
    close(conn);
}

int net_recv() {
    static int c;
    c = read(conn, sbuf, 512);

    if (c == -1) {
        switch (errno) {
        case ECONNRESET:
        case ENOTCONN:
        case ETIMEDOUT:
            fprintf(stderr, "error: connection closed\n");
            break;
        }
    }

    return c;
}
