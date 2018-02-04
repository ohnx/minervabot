#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include "net.h"
#include "irc.h"
#include "modules.h"

volatile sig_atomic_t done = 0;
 
void term(int signum) {
    done = 1;
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

    modules_init();
    modules_rescan();

    /* catch sigterms */
    memset(&action, 0, sizeof(struct sigaction));
    action.sa_handler = term;
    sigaction(SIGTERM, &action, NULL);

    /* network stuff */
loop_retry:
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

    if (done) {
        /* program received SIGTERM, clean up modules */
        fprintf(stderr, "Cleaning up...\n");
        modules_destroy();
        return 0;
    }

    goto loop_retry;

    return -__LINE__;
}
