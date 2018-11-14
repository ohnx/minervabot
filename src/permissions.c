#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <limits.h>
#include <pthread.h>
#include "hashmap.h"
#include "logger.h"
#include "module.h"
#include "permissions.h"

hashmap *perm_map;
pthread_mutex_t permissions_mutex = PTHREAD_MUTEX_INITIALIZER;
char pbuf[128];

int permissions_init() {
    const char *owner_host;
    perm_map = hashmap_new();
    if (!perm_map) {
        logger_log(ERROR, "permissions", "OOM error when initializing");
        return -1;
    }

    /* get owner host */
    if ((owner_host = getenv("BOT_OWNER_HOST"))) {
        hashmap_put(perm_map, owner_host, 100000);
    } else {
        logger_log(WARN, "permissions", "`BOT_OWNER_HOST` not found; there will be no users with permissions.");
    }

    /* TODO - load more permissions */
    return 0;
}

void permissions_set(const char *host, int level) {
    pthread_mutex_lock(&permissions_mutex);
    if (hashmap_get(perm_map, host) != 0) hashmap_remove(perm_map, host);
    hashmap_put(perm_map, host, level);
    pthread_mutex_unlock(&permissions_mutex);
}

int permissions_get(const char *host) {
    int perm, len;
    pthread_mutex_lock(&permissions_mutex);
    perm = 0;
    len = strlen(host);
    if (len > 128) return 0;
    memcpy(pbuf, host, len + 1);

    logger_log(INFO, "permissions", "checking perms for %s", pbuf);
    while (!(perm = hashmap_get(perm_map, pbuf)) && len >= 0) {
        pbuf[len] = '*';
        pbuf[len+1] = '\0';
        len--;
    }
    pthread_mutex_unlock(&permissions_mutex);

    return perm;
}

void permissions_cleanup() {
    hashmap_destroy(perm_map);
}
