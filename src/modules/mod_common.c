#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>
#include "module.h"
#include "mod_common.h"

static size_t buffer_append(void *contents, size_t size, size_t nmemb, void *userp) {
    size_t realsize = size * nmemb;
    struct buffer *mem = (struct buffer *)userp;

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

CURLcode send_http_request(const char *url, struct buffer *buf) {
    CURL *curl_handle;
    CURLcode res;

    buf->memory = malloc(1);  /* will be grown as needed by the realloc above */ 
    buf->size = 0;    /* no data at this point */ 

    /* init the curl session */ 
    curl_handle = curl_easy_init();

    /* set the URL */
    curl_easy_setopt(curl_handle, CURLOPT_URL, url);

    /* send all data to this function  */ 
    curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, buffer_append);

    /* we pass our 'chunk' struct to the callback function */ 
    curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, (void *)buf);

    /* some servers don't like requests that are made without a user-agent
     field, so we provide one */ 
    curl_easy_setopt(curl_handle, CURLOPT_USERAGENT, "libcurl/1.0");

    /* get it! */ 
    res = curl_easy_perform(curl_handle);

    /* cleanup */
    curl_easy_cleanup(curl_handle);

    /* check for errors */ 
    if(res != CURLE_OK) {
        free(buf->memory);
    }

    return res;
}

int module_init(struct core_ctx *core) { return 0; }

void module_cleanup() {}
