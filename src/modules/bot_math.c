#include <stdio.h>
#include <string.h>
#include "module.h"
#include "api.h"
#include "queue.h"
#include "syard.h"
#include "rpn_calc.h"

static struct core_ctx *ctx;

#define CALCCMD "calc"
#define EVALCMD "eval"

static double ans = 0;

static double variable_resolver(void *ctx, const char* name) {
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

static char *func_wl[] = {
    "sin", "cos", "tan", "asin", "acos", "atan", "atan2", "exp", "exp2",
    "exp10", "log", "log10", "log2", "logb", "pow", "sqrt", "cbrt", "hypot",
    "expm1", "log1p", "sinh", "cosh", "tanh", "asinh", "acosh", "atanh", "erf",
    "erfc", "lgamma", "gamma", "tgamma", "j0", "j1", "y0", "y1", "fabs", "fmod", NULL};

int check_func_wl(const char *f) {
    char **wl = func_wl;

    /* loop while the elements aren't null */
    while (*wl != NULL) {
        /* check if the strings are equal, and if yes, return allowed */
        if (!strcmp(*(wl++), f)) return 1;
    }

    /* default: function not found, deny */
    return 0;
}

double *function_resolver(void *ctx, const char *name, size_t argc, double *argv) {
    double *nbr_ptr;

    /* first check if it is a libm function */
    if (check_func_wl(name)) {
        /* call into libleo run_function */
        nbr_ptr = run_function((leo_api *)ctx, name, argc, argv);
        return nbr_ptr;
    }

    /* function not found */
    ((leo_api *)ctx)->error = ESTR_FUNC_NOT_FOUND;
    return NULL;
}

static int handle_calc(const char *cmdname, struct command_sender who, char *where, char *args) {
    queue *q;
    double *r;
    leo_api lapi;

    lapi.variable_resolver = &variable_resolver;
    lapi.function_resolver = &function_resolver;
    lapi.error = NULL;
    lapi.variable_resolver_ctx = NULL;
    lapi.function_resolver_ctx = &lapi;

    q = syard_run(&lapi, args);
    if (!q) {
        ctx->msgva(where, "%s: Error parsing expression: %s", who.nick, lapi.error);
        return 0;
    }

    r = rpn_calc(&lapi, q);

    if (!r) {
        ctx->msgva(where, "%s: Error calculating expression: %s", who.nick, lapi.error);
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
