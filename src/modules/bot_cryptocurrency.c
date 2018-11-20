#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <curl/curl.h>
#include <jsmn.h>
#include "module.h"

struct core_ctx *ctx;

#define BTCCMD "btc"
#define ETHCMD "eth"

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
 
  struct MemoryStruct chunk;
 
  chunk.memory = malloc(1);  /* will be grown as needed by the realloc above */ 
  chunk.size = 0;    /* no data at this point */ 
 
  curl_global_init(CURL_GLOBAL_ALL);
 
  /* init the curl session */ 
  curl_handle = curl_easy_init();
 
  /* specify URL to get */ 
  curl_easy_setopt(curl_handle, CURLOPT_URL, "https://api.coindesk.com/v1/bpi/currentprice.json");
 
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
	double btc_price, request_amnt;
	char *btc_price_a;
    jsmn_parser p;
	jsmntok_t t[128]; /* We expect no more than 128 tokens */

    ctx->log(INFO, "bot_cryptocurrency", "%lu bytes retrieved. Parsing...", (unsigned long)chunk.size);

	jsmn_init(&p);
	r = jsmn_parse(&p, chunk.memory, chunk.size, t, sizeof(t)/sizeof(t[0]));
	if (r < 0) {
		ctx->log(WARN, "bot_cryptocurrency", "Failed to parse JSON: %d\n", r);
		return 1;
	}

	/* Assume the top-level element is an object */
	if (r < 1 || t[0].type != JSMN_OBJECT) {
		ctx->log(WARN, "bot_cryptocurrency", "Failed to parse JSON: Object expected\n");
		return 1;
	}

    /* loop through array */
 	/*int i;
    for (i = 1; i < r; i++) {
        printf("%.*s\n", t[i].end-t[i].start, chunk.memory + t[i].start);
    }*/

    btc_price_a = strndup(chunk.memory + t[26].start, t[26].end-t[26].start);
    printf("btc_price_a: %s\n", btc_price_a);
    btc_price = atof(btc_price_a);
    free(btc_price_a);
    if (strlen(args) > 1) {
        /* args given */
        request_amnt = atof(args);
        ctx->msgva(where, "Current $BTC price: %lf USD. %lf BTC = %lf USD.", btc_price, request_amnt, request_amnt * btc_price);
    } else {
        /* no args */
        ctx->msgva(where, "Current $BTC price: %lf USD.", btc_price);
    }
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
    return ctx->register_cmd(BTCCMD, &handle_cmd);
}

void module_cleanup() {
    ctx->unregister_cmd(BTCCMD);
}
