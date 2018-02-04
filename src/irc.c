#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include "common.h"
#include "net.h"
#include "irc.h"
#include "modules.h"

void irc_filter(char *str) {
    int len = strlen(str), i;
    /* remove newlines from str */
    for (i = 0; i < len; i++) {
        if (str[i] == '\r' || str[i] == '\n') str[i] = ' ';
    }
}

void irc_message(const char *chan, const char *message) {
    net_raw("PRIVMSG %s :%s\r\n", chan, message);
}

void irc_message_va(const char *chan, const char *fmt, ...) {
    char *tmp;
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(sbuf, 512, fmt, ap);
    va_end(ap);
    tmp = strdup(sbuf);
    if (tmp) irc_message(chan, tmp);
    free(tmp);
}

void irc_action(const char *chan, const char *message) {
    net_raw("PRIVMSG %s :\001ACTION %s\001\r\n", chan, message);
}

void irc_join(const char *chan) {
    net_raw("JOIN %s\r\n", chan);
}

void irc_part(const char *chan, const char *message) {
    net_raw("PART %s :%s\r\n", chan, message);
}

void irc_chmode(const char *chan, const char *modes) {
    net_raw("MODE %s %s\r\n", chan, modes);
}

void irc_kick(const char *chan, const char *nick, const char *reason) {
    if (reason)
        net_raw("KICK %s %s :%s\r\n", chan, nick, reason);
    else
        net_raw("KICK %s %s\r\n", chan, nick);
}

void irc_raw(const char *message) {
    net_raw("%s\r\n", message);
}

int irc_login(const char *user, const char *realname, const char *nick_t) {
    int login_success = 0, nicklen = strlen(nick_t), i, rl;

    /* we may need to make changes to nick */
    char *nick = malloc((nicklen < 20 ? 20 : nicklen) + 1);
    if (nick == NULL) return 0; /* OOM */
    memcpy(nick, nick_t, nicklen+1);

    net_raw("USER %s 0 0 :%s\r\n", user, realname);
    net_raw("NICK %s\r\n", nick);

    while ((rl = net_recv()) > 2) {
        sbuf[rl] = 0;
        printf(">> %s", sbuf);
        /* handle initial codes */
        for (i = 1; i < rl; i++) {
            if (sbuf[i-1] != ' ') continue;
            if (sbuf[i] == '0') {
                /* sign in OK */
                login_success = 1;
            } else if (sbuf[i] == '4') {
                /* Nickname error */
                if (i+2 < rl && sbuf[i+2] == '3' && nicklen < 20) {
                    /* Nick already in use, append a _ */
                    nick[nicklen++] = '_';
                    net_raw("NICK %s\r\n", nick);
                } else {
                    /* some other erroneous nickname; send a random nick */
                    net_raw("NICK bphsd\r\n");
                }
            }
        }
        if (login_success) break;
    }

    free(nick);
    return login_success;
}

void irc_loop() {
    char buf[513];
    char *user, *command, *where, *message, *target;
    int i, j, l, sl, o = -1, start, wordcount;

    while (!done && (sl = net_recv())) {
        for (i = 0; i < sl; i++) {
            o++;
            buf[o] = sbuf[i];
            if ((i > 0 && sbuf[i] == '\n' && sbuf[i - 1] == '\r') || o == 512) {
                buf[o == 512 ? o + 1 : o - 1] = '\0';
                l = o;
                o = -1;

                printf(">> %s\n", buf);

                if (!strncmp(buf, "PING", 4)) {
                    buf[1] = 'O';
                    net_raw("%s\r\n", buf);
                } else if (buf[0] == ':') {
                    wordcount = 0;
                    user = command = where = message = NULL;
                    for (j = 1; j < l; j++) {
                        if (buf[j] == ' ') {
                            buf[j] = '\0';
                            wordcount++;
                            switch(wordcount) {
                                case 1: user = buf + 1; break;
                                case 2: command = buf + start; break;
                                case 3: where = buf + start; break;
                            }
                            if (j == l - 1) continue;
                            start = j + 1;
                        } else if (buf[j] == ':' && wordcount == 3) {
                            if (j < l - 1) message = buf + j + 1;
                            break;
                        }
                    }

                    if (wordcount < 2) continue;

                    if (!strncmp(command, "PRIVMSG", 7) || !strncmp(command, "NOTICE", 6)) {
                        if (where == NULL || message == NULL) continue;
                        /* want full user if ((sep = strchr(user, '!')) != NULL) user[sep - user] = '\0'; */
                        if (where[0] == '#' || where[0] == '&' || where[0] == '+' || where[0] == '!') target = where; else target = user;
                        fprintf(stderr, "[from: %s] [reply-with: %s] [where: %s] [reply-to: %s] %s\n", user, command, where, target, message);
                        /* check command prefix */
                        modules_check_cmd(user, where, message);
                    }
                }
            }
        }
    }

    /* send quit */
    irc_raw("QUIT :Gone fishing.");
}
