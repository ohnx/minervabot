#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include "net.h"
#include "irc.h"
#include "modules.h"
#include "permissions.h"

volatile sig_atomic_t done = 0;
volatile sig_atomic_t srel = 0;

void cleanup(int signum) {
    if (signum == SIGUSR1) srel = 1;
    else done = 1;
}

int main(int argc, char **argv) {
    struct sigaction action;
    int i, fst = 0;
    const char *nick = getenv("BOT_NICK");
    const char *user = getenv("BOT_USER");
    const char *name = getenv("BOT_NAME");
    const char *host = getenv("BOT_NETWORK_HOST");
    const char *port = getenv("BOT_NETWORK_PORT");
    const char *nickserv = getenv("BOT_NICKSERV");
    cmdprefix = getenv("BOT_PREFIX");

    if (nick == NULL || host == NULL || port == NULL) {
        fprintf(stderr, "error: nick, host, or port not specified.\n");
        return -__LINE__;
    }

    if (user == NULL) user = nick;
    if (name == NULL) name = nick;

    if (argc == 1) fprintf(stderr, "warning: not joining any channels\n");

    permissions_init();

    modules_init();
    modules_rescanall();

    /* catch sigterms */
    memset(&action, 0, sizeof(struct sigaction));
    action.sa_handler = cleanup;
    sigaction(SIGTERM, &action, NULL);
    sigaction(SIGHUP, &action, NULL);
    sigaction(SIGINT, &action, NULL);
    sigaction(SIGUSR1, &action, NULL);

    /* network stuff */
loop_retry:
    if (done) {
        /* program received SIGTERM, clean up modules */
        fprintf(stderr, "Cleaning up...\n");
        modules_deinit();
        permissions_cleanup();
        return 0;
    }

    /* Wait a few (10) seconds before trying to reconnect */
    if (fst++) usleep(10000000);

    net_connect(host, port);

    if (!irc_login(user, name, nick)) {
        fprintf(stderr, "error: failed to connect to server.\n");
        goto loop_retry;
    }

    if (nickserv) {
        irc_raw(nickserv);
        /* nickserv is slow... needs 8 seconds to cloak, per my tests */
        usleep(8000000);
    }

    for (i = 1; i < argc; i++) {
        irc_join(argv[i]);
    }

    irc_loop();
    net_disconnect();

    goto loop_retry;

    return -__LINE__;
}
