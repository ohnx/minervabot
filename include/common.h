#ifndef __DEBUG_H
#define __DEBUG_H

#include <stdio.h>
#define __code_error(x) do {fprintf(stderr, "Error in file %s line %d: %s\n", __FILE__, __LINE__, x); exit(1);} while (0)

#include <signal.h>
extern volatile sig_atomic_t done;
extern volatile sig_atomic_t srel;
extern int verbosity;

#endif /* __DEBUG_H */
