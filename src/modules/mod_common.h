#include <curl/curl.h>

struct buffer {
    char *memory;
    size_t size;
};

CURLcode send_http_request(const char *url, struct buffer *buf);
