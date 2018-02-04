#ifndef __PERMISSIONS_INC
#define __PERMISSIONS_INC
#include "module.h"

void permissions_init();
void permissions_set(struct core_ctx *ctx, const char *host, int level);
int permissions_get(struct core_ctx *ctx, const char *host);
void *permissions_getmap();
void permissions_cleanup();

#endif /* __PERMISSIONS_INC */
