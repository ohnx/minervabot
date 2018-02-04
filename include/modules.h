#ifndef __MODULES_H
#define __MODULES_H

extern const char *cmdprefix;

void modules_check_cmd(char *from, char *where, char *message);
void modules_rescan();
void modules_destroy();
void modules_init();

#endif /* __MODULES_H */
