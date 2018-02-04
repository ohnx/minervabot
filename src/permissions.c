#include <stdlib.h>
#include <unistd.h>
#include <limits.h>
#include "hashmap.h"
#include "logger.h"
#include "permissions.h"

hashmap *perm_map;
/* without this magic variable, somehow, the value of perm_map will change... not sure why. */
void *magic_variable;

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
    return hashmap_get(perm_map, host);
}

void permissions_cleanup() {
    hashmap_destroy(perm_map);
}
