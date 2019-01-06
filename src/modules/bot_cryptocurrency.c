#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <curl/curl.h>
#include <jsmn.h>
#include "module.h"
#include "mod_common.h"

static struct core_ctx *ctx;

#define BTCCMD "BTC"
#define ETHCMD "ETH"
#define XMRCMD "XMR"
#define USDCMD "USD"
#define CADCMD "CAD"

static int handle_cmd(const char *cmdname, struct command_sender who, char *where, char *args) {
    CURLcode res;
    const char *url;
    char sym[4];
    struct buffer chunk;

    memcpy(sym, cmdname, 4);
    sym[3] = '\0';
    /* to uppercase converstion */
    if (sym[0] > 'Z') sym[0] -= 0x20;
    if (sym[1] > 'Z') sym[1] -= 0x20;
    if (sym[2] > 'Z') sym[2] -= 0x20;

    /* specify URL to get */
    if (!strcmp(sym, BTCCMD)) {
        url = "https://min-api.cryptocompare.com/data/price?fsym=BTC&tsyms=BTC,ETH,XMR,USD,CAD";
    } else if (!strcmp(sym, ETHCMD)) {
        url = "https://min-api.cryptocompare.com/data/price?fsym=ETH&tsyms=BTC,ETH,XMR,USD,CAD";
    } else if (!strcmp(sym, XMRCMD)) {
        url = "https://min-api.cryptocompare.com/data/price?fsym=XMR&tsyms=BTC,ETH,XMR,USD,CAD";
    } else if (!strcmp(sym, USDCMD)) {
        url = "https://min-api.cryptocompare.com/data/price?fsym=USD&tsyms=BTC,ETH,XMR,USD,CAD";
    } else if (!strcmp(sym, CADCMD)) {
        url = "https://min-api.cryptocompare.com/data/price?fsym=CAD&tsyms=BTC,ETH,XMR,USD,CAD";
    } else {
        /* some sort of programming error */
        ctx->log(WARN, "bot_cryptocurrency", "??? weird error.");
    }

    /* send the HTTP request */ 
    if ((res = send_http_request(url, &chunk)) != CURLE_OK) {
        ctx->log(WARN, "bot_cryptocurrency", "curl_easy_perform() failed: %s", curl_easy_strerror(res));
        ctx->msg(where, "Error fetching price information.");
        return 0;
    }
    /*
     * Now, our chunk.memory points to a memory block that is chunk.size
     * bytes big and contains the remote file.
     *
     * Do something nice with it!
     */ 
    int r;
    double btc_price = 0, eth_price = 0, xmr_price = 0, usd_price = 0, cad_price = 0, request_amnt;
    char *price_a;
    jsmn_parser p;
    jsmntok_t t[128]; /* We expect no more than 128 tokens */

    ctx->log(INFO, "bot_cryptocurrency", "%lu bytes retrieved. Parsing...", (unsigned long)chunk.size);

    jsmn_init(&p);
    r = jsmn_parse(&p, chunk.memory, chunk.size, t, sizeof(t)/sizeof(t[0]));
    if (r < 0) {
    	ctx->log(WARN, "bot_cryptocurrency", "Failed to parse JSON: %d", r);
    	return 1;
    }

    /* Assume the top-level element is an object */
    if (r < 1 || t[0].type != JSMN_OBJECT) {
    	ctx->log(WARN, "bot_cryptocurrency", "Failed to parse JSON: Object expected");
    	return 1;
    }

    price_a = strndup(chunk.memory + t[2].start, t[2].end-t[2].start);
    btc_price = atof(price_a);
    free(price_a);

    price_a = strndup(chunk.memory + t[4].start, t[4].end-t[4].start);
    eth_price = atof(price_a);
    free(price_a);

    price_a = strndup(chunk.memory + t[6].start, t[6].end-t[6].start);
    xmr_price = atof(price_a);
    free(price_a);

    price_a = strndup(chunk.memory + t[8].start, t[8].end-t[8].start);
    usd_price = atof(price_a);
    free(price_a);

    price_a = strndup(chunk.memory + t[10].start, t[10].end-t[10].start);
    cad_price = atof(price_a);
    free(price_a);

    if (strlen(args) > 1) {
        /* args given */
        request_amnt = atof(args);
    } else {
        request_amnt = 1;
    }

    ctx->msgva(where, "%lf %s = %lf BTC / %lf ETH / %lf XMR / %.2lf USD / %.2lf CAD", request_amnt, sym,
                request_amnt * btc_price, request_amnt * eth_price, request_amnt * xmr_price,
                request_amnt * usd_price, request_amnt * cad_price);

    free(chunk.memory);
    return 0;
}

int module_init(struct core_ctx *core) {
    ctx = core;
    return ctx->register_cmd(BTCCMD, &handle_cmd) +
    ctx->register_cmd(ETHCMD, &handle_cmd) +
    ctx->register_cmd(XMRCMD, &handle_cmd) +
    ctx->register_cmd(USDCMD, &handle_cmd) +
    ctx->register_cmd(CADCMD, &handle_cmd)
    ;
}

void module_cleanup() {
    ctx->unregister_cmd(BTCCMD);
    ctx->unregister_cmd(ETHCMD);
    ctx->unregister_cmd(XMRCMD);
    ctx->unregister_cmd(USDCMD);
    ctx->unregister_cmd(CADCMD);
}
