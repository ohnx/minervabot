#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <curl/curl.h>
#include <jsmn.h>
#include "module.h"
#include "mod_common.h"

static struct core_ctx *ctx;
static const char *api_key;

#define WOLFRAMALPHACMD "wolframalpha"
#define WACMD "wa"
#define WOLFCMD "wolf"

int wa_handle_cmd(const char *cmdname, struct command_sender who, char *where, char *args) {
    char *curl_buf;
    char url[768];
    CURL *curl_handle;
    CURLcode res;
    struct buffer buf;

    /* escape the url arguments using curl */
    curl_handle = curl_easy_init();
    curl_buf = curl_easy_escape(curl_handle, args, 0);
    snprintf(url, 768, "https://api.wolframalpha.com/v1/result?i=%s&appid=%s", curl_buf, api_key);
    curl_free(curl_buf);
    curl_easy_cleanup(curl_handle);

    /* send the HTTP request */
    if ((res = send_http_request(url, &buf)) != CURLE_OK) {
        ctx->log(WARN, "bot_wolfram", "send_http_request() failed: %s", curl_easy_strerror(res));
        ctx->msg(where, "Error calling WolframAlpha API.");
    } else {
        ctx->msgva(where, "%s: %s", who.nick, buf.memory);
        free(buf.memory);
    }

    return 0;
}

int module_init(struct core_ctx *core) {
    ctx = core;
    api_key = getenv("BOT_MODULE_WOLFRAM_KEY");
    if (!api_key) {
        ctx->log(ERROR, "bot_wolfram", "Please specify a WolframAlpha API key");
        return -1;
    }
    return ctx->register_cmd(WOLFRAMALPHACMD, &wa_handle_cmd) +
    ctx->register_cmd(WACMD, &wa_handle_cmd) +
    ctx->register_cmd(WOLFCMD, &wa_handle_cmd)
    ;
}

void module_cleanup() {
    ctx->unregister_cmd(WOLFRAMALPHACMD);
    ctx->unregister_cmd(WACMD);
    ctx->unregister_cmd(WOLFCMD);
}
