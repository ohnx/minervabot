#include <stdio.h>
#include <string.h>
#include "module.h"
#include "queue.h"
#include "syard.h"
#include "rpn_calc.h"

struct core_ctx *ctx;

#define CALCCMD "calc"
#define EVALCMD "eval"

double ans = 0;

char *func_wl[] = {
    "sin", "cos", "tan", "asin", "acos", "atan", "atan2", "exp", "exp2",
    "exp10", "log", "log10", "log2", "logb", "pow", "sqrt", "cbrt", "hypot",
    "expm1", "log1p", "sinh", "cosh", "tanh", "asinh", "acosh", "atanh", "erf",
    "erfc", "lgamma", "gamma", "tgamma", "j0", "j1", "y0", "y1", "fabs", "fmod", NULL};

double variable_resolver(const char* name) {
    switch (*name) {
    case 'e':
        if (!name[1]) return 2.7182818285;
        break;
    case 'p':
        if (name[1] == 'i' && !name[2]) return 3.1415926535;
        break;
    case 'a':
        if (name[1] == 'n' && name[2] == 's' && !name[3]) return ans;
        break;
    }

    return 0;
}

int handle_calc(const char *cmdname, struct command_sender who, char *where, char *args) {
    queue *q;
    double *r;

    q = syard_run(args);
    if (!q) {
        ctx->msgva(where, "%s: Error parsing expression", who.nick);
        return 0;
    }

    r = rpn_calc(q, variable_resolver, func_wl);

    if (!r) {
        ctx->msgva(where, "%s: Error calculating expression", who.nick);
        return 0;
    }

    ctx->msgva(where, "%s: %g", who.nick, *r);

    ans = *r;
    free(r);

    return 0;
}

int module_init(struct core_ctx *core) {
    ctx = core;

    return ctx->register_cmd(CALCCMD, &handle_calc) +
           ctx->register_cmd(EVALCMD, &handle_calc);
}

void module_cleanup() {
    ctx->unregister_cmd(CALCCMD);
    ctx->unregister_cmd(EVALCMD);
}
