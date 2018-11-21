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
    char buf[513];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(buf, 512, fmt, ap);
    va_end(ap);
    irc_message(chan, buf);
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

void irc_parsesender(struct command_sender *sender, char *str) {
    char *t;

    sender->nick = str;

    /* set ident */
    t = strchr(str, '!');
    if (t) *(t++) = '\0';
    sender->ident = t;

    /* set host */
    t = strchr(t ? t : "", '@');
    if (t) *(t++) = '\0';
    sender->host = t;
}

int irc_login(const char *user, const char *realname, const char *nick_t, const char *password) {
    char buf[513];
    int login_success = 0, nicklen = strlen(nick_t), i, rl, j, o = 0;

    /* we may need to make changes to nick */
    char *nick = malloc((nicklen < 20 ? 20 : nicklen) + 1);
    if (nick == NULL) return 0; /* OOM */
    memcpy(nick, nick_t, nicklen+1);

    logger_log(INFO, "irc", "connected to server, logging in...");

    /* check if password given */
    if (password) net_raw("PASS %s\r\n", password);

    /* send USER and NICK stuff */
    net_raw("USER %s 0 0 :%s\r\n", user, realname);
    net_raw("NICK %s\r\n", nick);

    /* Loop to ensure NICK was valid */
    while ((rl = net_recv()) > 2) {
        for (j = 0; j < rl; j++) {
            buf[o] = rbuf[j];
            if ((j > 0 && rbuf[j] == '\n' && rbuf[j - 1] == '\r') || o == 512) {
                /* found a line */
                buf[o] = '\0';
                /* handle initial codes */
                for (i = 1; i < o; i++) {
                    if (buf[i-1] != ' ') {
                        if (buf[1] == 'I') {
                            /* some ircd's send pings so we have to handle them */
                            buf[1] = 'O';
                            net_raw("%s\r\n", buf);
                        } else {
                            continue;
                        }
                    }
                    if (buf[i] == '0') {
                        /* sign in OK */
                        login_success = 1;
                    } else if (buf[i] == '4') {
                        /* Nickname error */
                        if (i+2 < rl && buf[i+2] == '3' && nicklen < 20) {
                            /* Nick already in use, append a _ */
                            nick[nicklen++] = '_';
                            nick[nicklen] = '\0';
                            net_raw("NICK %s\r\n", nick);
                        } else {
                            /* some other erroneous nickname; send a random nick */
                            net_raw("NICK bphsd\r\n");
                        }
                    }
                }
                o = 0;
                continue;
            }
            o++;
        }
        if (login_success) break;
    }

    free(nick);
    logger_log(INFO, "irc", "successfully identified to server");
    return login_success;
}

void irc_loop() {
    char buf[513];
    char nickbuf[513];
    char *user, *command, *where, *message, *target;
    int i, j, l, sl, o = -1, start, wordcount;

    while (!done && (sl = net_recv())) {
        for (i = 0; i < sl; i++) {
            o++;
            buf[o] = rbuf[i];
            if ((i > 0 && rbuf[i] == '\n' && rbuf[i - 1] == '\r') || o == 512) {
                buf[o == 512 ? o + 1 : o - 1] = '\0';
                l = o;
                o = -1;

                if (verbosity >= 2) printf(">> %s\n", buf);

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

                    if (wordcount < 2 || !command) continue;

                    if (!strncmp(command, "PRIVMSG", 7) || !strncmp(command, "NOTICE", 6)) {
                        if (where == NULL || message == NULL) continue;
                        /* want full user if ((sep = strchr(user, '!')) != NULL) user[sep - user] = '\0'; */
                        if (where[0] == '#' || where[0] == '&' || where[0] == '+' || where[0] == '!') target = where; else {
                            /* remove the nick */
                            char *sep = strchr(user, '!');

                            if (sep == NULL) {
                                target = user;
                            } else {
                                /* copy to buffer */
                                memcpy(nickbuf, user, sep - user);
                                nickbuf[sep - user] = '\0';
                                target = nickbuf;
                            }
                        }
                        if (verbosity >= 1) printf("%s <%s> %s\n", where, user, message);
                        /* check command prefix */
                        modules_check_cmd(user, target, message);
                    } else if (!strncmp(command, "QUIT", 4)) {
                        /* special hax */
                        if (!strncmp(user, "WarsawBot", 9)) {
                            irc_message("##lazy-valoran", "=dammitwarsaw");
                        }
                    }
                }
            }
        }

        /* This is super ugly - reloading should be handled in main.c */
        if (srel) {
            srel = 0;
            modules_rescanall();
        }
    }

    /* send quit */
    irc_raw("QUIT :Gone fishing.");
}
