#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <curl/curl.h>
#include <jsmn.h>
#include "module.h"

struct core_ctx *ctx;

#define BTCCMD "BTC"
#define ETHCMD "ETH"
#define XMRCMD "XMR"
#define USDCMD "USD"
#define CADCMD "CAD"

struct MemoryStruct {
  char *memory;
  size_t size;
};
 
static size_t WriteMemoryCallback(void *contents, size_t size, size_t nmemb, void *userp) {
  size_t realsize = size * nmemb;
  struct MemoryStruct *mem = (struct MemoryStruct *)userp;
 
  char *ptr = realloc(mem->memory, mem->size + realsize + 1);
  if (ptr == NULL) {
    /* out of memory! */ 
    printf("not enough memory (realloc returned NULL)\n");
    return 0;
  }
 
  mem->memory = ptr;
  memcpy(&(mem->memory[mem->size]), contents, realsize);
  mem->size += realsize;
  mem->memory[mem->size] = 0;
 
  return realsize;
}


int handle_cmd(const char *cmdname, struct command_sender who, char *where, char *args) {
  CURL *curl_handle;
  CURLcode res;
  const char *url;
  char sym[4];

  struct MemoryStruct chunk;
 
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

  chunk.memory = malloc(1);  /* will be grown as needed by the realloc above */ 
  chunk.size = 0;    /* no data at this point */ 
 
  curl_global_init(CURL_GLOBAL_ALL);
 
  /* init the curl session */ 
  curl_handle = curl_easy_init();

  curl_easy_setopt(curl_handle, CURLOPT_URL, url);
 
  /* send all data to this function  */ 
  curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);
 
  /* we pass our 'chunk' struct to the callback function */ 
  curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, (void *)&chunk);
 
  /* some servers don't like requests that are made without a user-agent
     field, so we provide one */ 
  curl_easy_setopt(curl_handle, CURLOPT_USERAGENT, "libcurl/1.0");
 
  /* get it! */ 
  res = curl_easy_perform(curl_handle);
 
  /* check for errors */ 
  if(res != CURLE_OK) {
    ctx->log(WARN, "bot_cryptocurrency", "curl_easy_perform() failed: %s", curl_easy_strerror(res));
    ctx->msg(where, "Error fetching price information.");
  } else {
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

    /* loop through array */
 	/*int i;
    for (i = 1; i < r; i++) {
        printf("%.*s\n", t[i].end-t[i].start, chunk.memory + t[i].start);
    }*/

    /*printf("%d\n", r);*/

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
  }
 
  /* cleanup curl stuff */ 
  curl_easy_cleanup(curl_handle);
 
  free(chunk.memory);
 
  /* we're done with libcurl, so clean it up */ 
  curl_global_cleanup();
 
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
