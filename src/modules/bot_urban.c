#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <curl/curl.h>
#include "module.h"
#include "mod_common.h"

static struct core_ctx *ctx;

#define URBANCMD "urban"
#define UDCMD "ud"
#define N_MAX 400

static int handle_cmd(const char *cmdname, struct command_sender who, char *where, char *args) {
    CURL *curl_handle;
    CURLcode res;
    char *curl_buf, *last_idx;
    int n;
    char url[512];
    struct buffer buf;

    /* escape the url arguments using curl */
    curl_handle = curl_easy_init();
    curl_buf = curl_easy_escape(curl_handle, args, 0);
    snprintf(url, 512, "http://api.urbandictionary.com/v0/define?term=%s", curl_buf);
    curl_free(curl_buf);
    curl_easy_cleanup(curl_handle);

    /* send the HTTP request */
    if ((res = send_http_request(url, &buf)) != CURLE_OK) {
        ctx->log(WARN, "bot_urban", "send_http_request() failed: %s", curl_easy_strerror(res));
        ctx->msg(where, "Error calling Urban Dictionary API.");
    } else {
        ctx->log(INFO, "bot_urban", "%lu bytes retrieved. Parsing...", (unsigned long)buf.size);
        if (buf.size < 25) {
            ctx->msgva(where, "No definition found.");
            goto done;
        }

        url[0] = '\0';
        n = 0;
        /* definition begins at this index and goes until we reach an end */
        curl_buf = last_idx = &buf.memory[24];
        while (*curl_buf != '\0' && *curl_buf != '"' && (curl_buf-last_idx + n) < N_MAX) {
            if (*curl_buf == '\\') {
                /* escape codes */
                curl_buf++;
                if (*curl_buf == '\\' || *curl_buf == '"') {
                    memcpy(url+n, last_idx, curl_buf-last_idx-1); /* skip the last character */
                    n += curl_buf-last_idx;
                    last_idx = curl_buf+1; /* and skip the next */
                    url[n-1] = *curl_buf; /* but add the correct character */
                    url[n] = '\0';
                } else if (*curl_buf == 'r') {
                    memcpy(url+n, last_idx, curl_buf-last_idx-1); /* skip the last character */
                    n += curl_buf-last_idx-1;
                    last_idx = curl_buf+1; /* and skip the next */
                    url[n] = '\0';
                } else if (*curl_buf == 'n' || *curl_buf == 't') {
                    memcpy(url+n, last_idx, curl_buf-last_idx-1); /* skip the last character */
                    n += curl_buf-last_idx;
                    last_idx = curl_buf+1; /* and skip the next */
                    url[n-1] = ' '; /* but add the correct character */
                    url[n] = '\0';
                }
            }

            curl_buf++;
        }
        /* copy over then null terminate */
        memcpy(url+n, last_idx, curl_buf-last_idx);
        n += curl_buf-last_idx;
        url[n] = '\0';/* TODO
        if (n >= N_MAX) {
            strncat(url, "...", 3);
        }*/
        ctx->msgva(where, "\x02%s\x0f: %s", args, url);

    done:
        free(buf.memory);
    }

    return 0;
}

int module_init(struct core_ctx *core) {
    ctx = core;
    return ctx->register_cmd(URBANCMD, &handle_cmd) +
    ctx->register_cmd(UDCMD, &handle_cmd)
    ;
}

void module_cleanup() {
    ctx->unregister_cmd(URBANCMD);
    ctx->unregister_cmd(UDCMD);
}
