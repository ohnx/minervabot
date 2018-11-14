#ifndef __PERMISSIONS_INC
#define __PERMISSIONS_INC

int permissions_init();
void permissions_set(const char *host, int level);
int permissions_get(const char *host);
void permissions_cleanup();

#endif /* __PERMISSIONS_INC */
