#ifndef __MODULES_H
#define __MODULES_H

extern const char *cmdprefix;

void modules_check_cmd(char *from, char *where, char *message);
void modules_rescanall();
void modules_unloadall();
void modules_deinit();
void modules_init();

#endif /* __MODULES_H */
