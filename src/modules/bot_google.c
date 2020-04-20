#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <curl/curl.h>
#include "module.h"
#include "mod_common.h"

static struct core_ctx *ctx;
static const char *gdansk = "auto";

#define GOOGLETRANS "trans"
#define GTCMD "tr"
#define N_MAX 400

static int handle_cmd(const char *cmdname, struct command_sender who, char *where, char *args) {
    CURL *curl_handle;
    CURLcode res;
    char *curl_buf, *last_idx, *dest_lang, *orig_lang;
    int n;
    char url[1024];
    struct buffer chunk;

    /* figure out the dest language */
    if (!args || !(*args)) return 0;
    dest_lang = "en";
    orig_lang = (char *)gdansk;

    if (*args != ' ' && args[1] && args[1] != ' ' && args[2]) {
        if (args[2] == ' ') {
            /* 2-char dest lang then whatever */
            args[2] = '\0';
            dest_lang = args;
            args += 3;
        } else if (args[2] == ';' && args[3] && args[3] != ' ' && args[4] && args[4] != ' ' && args[5] && args[5] == ' ') {
            /* 2-char src lang then ';' then 2-char dest lang then whatever */
            args[2] = '\0';
            orig_lang = args;
            args += 3;
            args[2] = '\0';
            dest_lang = args;
            args += 3;
        }
    }

    /* init the curl session */ 
    curl_handle = curl_easy_init();

    /* set url. note that this does not work 100% with cjk, but idk why :( */
    curl_buf = curl_easy_escape(curl_handle, args, 0);
    snprintf(url, 1024, "https://translate.googleapis.com/translate_a/single?client=gtx&sl=%s&tl=%s&dt=t&q=%s", orig_lang, dest_lang, curl_buf);
    curl_free(curl_buf);

    /* check for errors */ 
    if ((res = send_http_request(url, &chunk)) != CURLE_OK) {
        ctx->log(WARN, "bot_google", "curl_easy_perform() failed: %s", curl_easy_strerror(res));
        ctx->msg(where, "Error fetching translation.");
        return 0;
    }
    /*
     * Now, our chunk.memory points to a memory block that is chunk.size
     * bytes big and contains the remote file.
     *
     * Do something nice with it!
     */ 
    ctx->log(INFO, "bot_google", "%lu bytes retrieved. Parsing...", (unsigned long)chunk.size);
    if (chunk.size < 30) {
        ctx->msgva(where, "Translation failed.");
        goto done;
    }

    url[0] = '\0';
    n = 0;
    /* definition begins at this index and goes until we reach an end */
    curl_buf = last_idx = &chunk.memory[4];
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

    /* also check the language stuff */
    if (orig_lang == gdansk) {
        orig_lang = chunk.memory + chunk.size - 1;
        while (*orig_lang != '[' && orig_lang > chunk.memory) orig_lang--;
        orig_lang += 2;
        last_idx = strchr(orig_lang, '"');
        *last_idx = '\0';
    }
    ctx->msgva(where, "%s to %s: %s", orig_lang, dest_lang, url);

done:
    /* cleanup curl stuff */ 
    free(chunk.memory);

    return 0;
}

int module_init(struct core_ctx *core) {
    ctx = core;
    return ctx->register_cmd(GOOGLETRANS, &handle_cmd) +
    ctx->register_cmd(GTCMD, &handle_cmd)
    ;
}

void module_cleanup() {
    ctx->unregister_cmd(GOOGLETRANS);
    ctx->unregister_cmd(GTCMD);
}
