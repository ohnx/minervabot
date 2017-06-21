#include <stdio.h>
#include "common.h"
#include "netabs.h"

static char sbuf[512];

int main() {
    char *channel = "##ohnx";
    char *host = "irc.freenode.net";
    char *port = "6667";
    
    char *user, *command, *where, *message, *sep, *target;
    int i, j, l, sl, o = -1, start, wordcount;
    char buf[513];
    
    
    
    /* single threaded for now */
    bot_context *bc;
    
    bc = bot_connect(host, port);
    
    while ((sl = read(bc->conn, sbuf, 512))) {
        for (i = 0; i < sl; i++) {
            o++;
            buf[o] = sbuf[i];
            if ((i > 0 && sbuf[i] == '\n' && sbuf[i - 1] == '\r') || o == 512) {
                buf[o + 1] = '\0';
                l = o;
                o = -1;
                
                printf(">> %s", buf);
                
                if (!strncmp(buf, "PING", 4)) {
                    buf[1] = 'O';
                    net_raw(bc, buf);
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
                    
                    if (!strncmp(command, "001", 3) && channel != NULL) {
                        bot_joinchan(bc, channel);
#ifdef POSTCONNECT_COMMAND
                        net_raw(bc, POSTCONNECT_COMMAND);
#endif
                    } else if (!strncmp(command, "433" ,3)) { // TODO: Test
                        unsigned int nicklen = strlen(bc->nick);
                        char *newnick = calloc(nicklen+2, sizeof(char)); // +1 for _ +1 for null
                        strcpy(newnick, bc->nick);
                        newnick[nicklen] = '_';
                        bot_changenick(bc, newnick);
                        free(newnick);
                    } else if (!strncmp(command, "PRIVMSG", 7) || !strncmp(command, "NOTICE", 6)) {
                        if (where == NULL || message == NULL) continue;
                        if ((sep = strchr(user, '!')) != NULL) user[sep - user] = '\0';
                        if (where[0] == '#' || where[0] == '&' || where[0] == '+' || where[0] == '!') target = where; else target = user;
                        printf("[from: %s] [reply-with: %s] [where: %s] [reply-to: %s] %s", user, command, where, target, message);
                    }
                }
                
            }
        }
        
    }
    
    return 0;
    
}