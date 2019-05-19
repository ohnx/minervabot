#ifndef __COMMON_H
#define __COMMON_H

#include <stdio.h>
#define __code_error(x) do {fprintf(stderr, "Error in file %s line %d: %s\n", __FILE__, __LINE__, x); exit(1);} while (0)

#include <signal.h>
extern volatile sig_atomic_t done;
extern int verbosity;

#endif /* __COMMON_H */
