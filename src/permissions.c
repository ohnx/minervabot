#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <limits.h>
#include "hashmap.h"
#include "logger.h"
#include "module.h"
#include "permissions.h"

hashmap *perm_map;
/* without this magic variable, somehow, the value of perm_map will change... not sure why. */
void *magic_variable;

char pbuf[128];

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

void permissions_set(const char *host, int level) {
    if (hashmap_get(perm_map, host) != 0) hashmap_remove(perm_map, host);
    hashmap_put(perm_map, host, level);
}

int permissions_get(const char *host) {
    int perm, len;
    perm = 0;
    len = strlen(host);
    memcpy(pbuf, host, len + 1);

    logger_log(INFO, "permissions", "checking perms for %s", pbuf);
    while (!(perm = hashmap_get(perm_map, pbuf)) && len >= 0) {
        pbuf[len] = '*';
        pbuf[len+1] = '\0';
        logger_log(INFO, "permissions", "checking perms for %s", pbuf);
        len--;
    }

    return perm;
}

void permissions_cleanup() {
    hashmap_destroy(perm_map);
}
