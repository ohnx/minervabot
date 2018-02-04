#ifndef __IRC_H
#define __IRC_H
#include "net.h"

void irc_filter(char *str);
void irc_message(const char *chan, const char *message);
void irc_action(const char *chan, const char *message);
void irc_join(const char *chan);
void irc_part(const char *chan, const char *message);
void irc_chmode(const char *chan, const char *modes);
void irc_kick(const char *chan, const char *nick, const char *reason);
void irc_raw(const char *message);
int irc_login(const char *user, const char *realname, const char *nick_t);
void irc_loop();

#endif /* __IRC_H */