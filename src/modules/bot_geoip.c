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

struct core_ctx *ctx;
const char *api_key;

#define GEOIPCMD "geoip"
#define IPCMD "ip"
#define HOSTCMD "host"

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
  int errcode;
  void *ptr;
  char url[512];
  char ip_buf[INET6_ADDRSTRLEN];
  struct addrinfo hints, *addr_lst;
  struct MemoryStruct chunk;

  memset (&hints, 0, sizeof (hints));
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_flags |= AI_CANONNAME;

  errcode = getaddrinfo(args, NULL, &hints, &addr_lst);
  if (errcode != 0) {
      ctx->msgva(where, "%s: Please specify a valid IP.", who.nick);
      freeaddrinfo(addr_lst);
      return 0;
  }

  if (addr_lst->ai_addr->sa_family == AF_INET) {
      ptr = &((struct sockaddr_in *) addr_lst->ai_addr)->sin_addr;
  } else {
      ptr = &((struct sockaddr_in6 *) addr_lst->ai_addr)->sin6_addr;
  }

  inet_ntop(addr_lst->ai_addr->sa_family, ptr, ip_buf, INET6_ADDRSTRLEN);

  snprintf(url, 512, "http://ipinfo.io/%s/json?token=%s", ip_buf, api_key);

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
    ctx->log(WARN, "bot_geoip", "curl_easy_perform() failed: %s", curl_easy_strerror(res));
    ctx->msg(where, "Error fetching price information.");
  } else {
    /*
     * Now, our chunk.memory points to a memory block that is chunk.size
     * bytes big and contains the remote file.
     *
     * Do something nice with it!
     */ 
	int r;
    jsmn_parser p;
	jsmntok_t t[128]; /* We expect no more than 128 tokens */

    ctx->log(INFO, "bot_geoip", "%lu bytes retrieved. Parsing...", (unsigned long)chunk.size);

	jsmn_init(&p);
	r = jsmn_parse(&p, chunk.memory, chunk.size, t, sizeof(t)/sizeof(t[0]));
	if (r < 0) {
		ctx->log(WARN, "bot_geoip", "Failed to parse JSON: %d", r);
		return 1;
	}

	/* Assume the top-level element is an object */
	if (r < 1 || t[0].type != JSMN_OBJECT) {
		ctx->log(WARN, "bot_geoip", "Failed to parse JSON: Object expected");
		return 1;
	}

    /* loop through array */
 	int i;
 	url[0] = '\0';
    for (i = 1; i < r; i++) {
        if (!strncmp("region", chunk.memory + t[i].start, t[i].end-t[i].start) ||
            !strncmp("country", chunk.memory + t[i].start, t[i].end-t[i].start)) {
        } else if (!strncmp("city", chunk.memory + t[i].start, t[i].end-t[i].start)) {
            strncat(url, "\x02Location\x0f: ", 12);
        } else if (!strncmp("loc", chunk.memory + t[i].start, t[i].end-t[i].start)) {
            strncat(url, "\002Coordinates\x0f: ", 15);
        } else if (!strncmp("org", chunk.memory + t[i].start, t[i].end-t[i].start)) {
            strncat(url, "\x02ISP\x0f: ", 7);
        } else if (!strncmp("ip", chunk.memory + t[i].start, t[i].end-t[i].start)) {
            strncat(url, "\x02IP\x0f: ", 6);
        } else {
            strncat(url, "\x02", 1);
            *(chunk.memory + t[i].start) -= 'a' - 'A';
            strncat(url, chunk.memory + t[i].start, t[i].end-t[i].start);
            *(chunk.memory + t[i].start) += 'a' - 'A';
            strncat(url, "\x0f: ", 3);
        }
        if (t[i+1].end == t[i+1].start) {
            strncat(url, "[unknown]", 9);
        } else {
            strncat(url, chunk.memory + t[i+1].start, t[i+1].end-t[i+1].start);
        }
        /* extras */
        if (!strncmp("country", chunk.memory + t[i].start, t[i].end-t[i].start)) {
            ip_buf[0] = ' ';
            ip_buf[1] = '\xf0';
            ip_buf[2] = '\x9f';
            ip_buf[3] = '\x87';
            ip_buf[4] = '\xa6' + *(chunk.memory + t[i+1].start) - 'A';
            ip_buf[5] = '\xf0';
            ip_buf[6] = '\x9f';
            ip_buf[7] = '\x87';
            ip_buf[8] = '\xa6' + *(chunk.memory + t[i+1].start + 1) - 'A';
            strncat(url, ip_buf, 9);
        }
        if (++i < r -1)
            strncat(url, ", ", 2);
        /* if we segfault here it's ok :) */
        //if (!strcmp())
    }
    ctx->msg(where, url);
  }
 
  /* cleanup curl stuff */ 
  curl_easy_cleanup(curl_handle);
 
  free(chunk.memory);
 
  /* we're done with libcurl, so clean it up */ 
  curl_global_cleanup();

  freeaddrinfo(addr_lst);

  return 0;
}

int module_init(struct core_ctx *core) {
    ctx = core;
    api_key = getenv("BOT_MODULE_GEOIP_KEY");
    if (!api_key) {
        ctx->log(ERROR, "bot_geoip", "Please specify an ipinfo.io API key");
        return -1;
    }
    return ctx->register_cmd(GEOIPCMD, &handle_cmd) +
    ctx->register_cmd(IPCMD, &handle_cmd) +
    ctx->register_cmd(HOSTCMD, &handle_cmd)
    ;
}

void module_cleanup() {
    ctx->unregister_cmd(GEOIPCMD);
    ctx->unregister_cmd(IPCMD);
    ctx->unregister_cmd(HOSTCMD);
}
