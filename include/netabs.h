#ifndef __MINERVA_ABSTRACTIONS_INC
#define __MINERVA_ABSTRACTIONS_INC

#include "config.h"
#include "common.h"
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <unistd.h>
#include <string.h>
#include <netdb.h>

bot_context *bot_connect(const char *host, const char *port);
void bot_changenick(bot_context *bc, const char *nick);
void initsetup(bot_context *bc);
void bot_joinchan(bot_context *bc, const char *chan);
void net_raw_va(bot_context *bc, const char *fmt, ...);
void net_raw(bot_context *bc, const char *str);


#endif
