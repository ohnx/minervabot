#include <stdlib.h>
#include <unistd.h>
#include <limits.h>
#include "hashmap.h"
#include "permissions.h"

int wtf = 0;
hashmap *perm_map;

#include <stdio.h>
void permissions_init() {
    const char *owner_host;
    perm_map = hashmap_new();

    /* get owner host */
    if ((owner_host = getenv("BOT_OWNER_HOST"))) {
        hashmap_put(perm_map, owner_host, 100000);
    } else {
        logger_log(WARN, "permissions", "`BOT_OWNER_HOST` not found; there will be no users with permissions.");
    }

    /* TODO - load more permissions */
}

void permissions_set(struct core_ctx *ctx, const char *host, int level) {
    hashmap_put(ctx->perm_map, host, level);
}

int permissions_get(struct core_ctx *ctx, const char *host) {
    return hashmap_get(ctx->perm_map, host);
}

void *permissions_getmap() {
    printf("perm map right now: %p\n", perm_map);
    return perm_map;
}

void permissions_cleanup() {
    printf("it changed again :/ %d %p\n", wtf, perm_map);
    hashmap_destroy(perm_map);
}
