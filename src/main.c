#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include "net.h"
#include "irc.h"
#include "modules.h"
#include "permissions.h"
#include "logger.h"

volatile sig_atomic_t done = 0;
int verbosity = 0;

void cleanup(int signum) {
    done = 1;
}

int main(int argc, char **argv) {
    struct sigaction action;
    pid_t pid;
    int i, fst = 0, use_ssl = 0;
    const char *nick = getenv("BOT_NICK");
    const char *user = getenv("BOT_USER");
    const char *name = getenv("BOT_NAME");
    const char *verbositya = getenv("BOT_LOG_VERBOSITY");
    const char *host = getenv("BOT_NETWORK_HOST");
    const char *port = getenv("BOT_NETWORK_PORT");
    const char *usessl = getenv("BOT_NETWORK_SSL");
    const char *netpw = getenv("BOT_NETWORK_PASSWORD");
    cmdprefix = getenv("BOT_PREFIX");

    if (nick == NULL || host == NULL || port == NULL) {
        logger_log(ERROR, "main", "nick, host, or port not specified");
        return -__LINE__;
    }

    pid = getpid();
    logger_log(INFO, "main", "my PID is %d", pid);

    /* default values */
    if (!user) user = nick;
    if (!name) name = nick;
    if (!usessl) usessl = "0";
    use_ssl = atoi(usessl);
    if (use_ssl != 1) use_ssl = 0;
    verbosity = atoi(verbositya ? verbositya : "1");
    if (verbosity > 3 || verbosity < 0) verbosity = 1;

    /* warnings */
    if (argc == 1) logger_log(WARN, "main", "not joining any channels");
    if (!use_ssl && netpw) logger_log(WARN, "main", "transmitting server password in cleartext");

    if (permissions_init()) return -__LINE__;
    if (modules_init()) return -__LINE__;
    modules_rescanall();

    /* catch signals */
    memset(&action, 0, sizeof(struct sigaction));
    sigemptyset(&action.sa_mask);
    action.sa_flags = SA_NODEFER;
    action.sa_handler = cleanup;
    sigaction(SIGTERM, &action, NULL);
    sigaction(SIGHUP, &action, NULL);
    sigaction(SIGINT, &action, NULL);

    /* network stuff */
loop_retry:
    if (done) {
        /* program received SIGTERM, clean up modules */
        logger_log(INFO, "main", "cleaning up...");
        modules_deinit();
        permissions_cleanup();
        logger_log(INFO, "main", "goodnight!");
        return 0;
    }

    /* Wait a few (10) seconds before trying to reconnect */
    if (fst++) usleep(10000000);

    /* if failed to connect, try again */
    if (net_connect(host, port, use_ssl)) goto loop_retry;

    /* attempt to log in */
    if (!irc_login(user, name, nick, netpw)) {
        logger_log(ERROR, "main", "failed to log in to server.");
        net_disconnect();
        goto loop_retry;
    }

    /* join all the channels */
    for (i = 1; i < argc; i++) {
        irc_join(argv[i]);
    }

    /* enter main irc loop */
    irc_loop();

    logger_log(WARN, "main", "bot disconnected from server");

    /* the loop quit for some reason... disconnect and try reconnecting! */
    net_disconnect();

    goto loop_retry;

    return -__LINE__;
}
