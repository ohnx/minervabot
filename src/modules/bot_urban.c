#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <curl/curl.h>
#include "module.h"

struct core_ctx *ctx;

#define URBANCMD "urban"
#define UDCMD "ud"
#define N_MAX 400

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
  char *curl_buf, *last_idx;
  int n;
  char url[512];
  struct MemoryStruct chunk;

  chunk.memory = malloc(1);  /* will be grown as needed by the realloc above */ 
  chunk.size = 0;    /* no data at this point */ 
 
  curl_global_init(CURL_GLOBAL_ALL);
 
  /* init the curl session */ 
  curl_handle = curl_easy_init();

  curl_buf = curl_easy_escape(curl_handle, args, 0);
  snprintf(url, 512, "http://api.urbandictionary.com/v0/define?term=%s", curl_buf);
  curl_free(curl_buf);

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
    ctx->log(WARN, "bot_urban", "curl_easy_perform() failed: %s", curl_easy_strerror(res));
    ctx->msg(where, "Error fetching definition.");
  } else {
    /*
     * Now, our chunk.memory points to a memory block that is chunk.size
     * bytes big and contains the remote file.
     *
     * Do something nice with it!
     */ 
    ctx->log(INFO, "bot_urban", "%lu bytes retrieved. Parsing...", (unsigned long)chunk.size);
    if (chunk.size < 25) {
        ctx->msgva(where, "No definition found.");
        goto done;
    }

    url[0] = '\0';
    n = 0;
    /* definition begins at this index and goes until we reach an end */
    curl_buf = last_idx = &chunk.memory[24];
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
  }
 
done:
  /* cleanup curl stuff */ 
  curl_easy_cleanup(curl_handle);
 
  free(chunk.memory);
 
  /* we're done with libcurl, so clean it up */ 
  curl_global_cleanup();

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
