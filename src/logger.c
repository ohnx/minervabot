#include <stdio.h>
#include <stdarg.h>
#include <pthread.h>
#include "logger.h"

#define STR_INFO "[\x1b[34mINFO\x1b[0m] "
#define STR_WARN "[\x1b[33mWARN\x1b[0m] "
#define STR_ERR  "[\x1b[31mERR \x1b[0m] "

pthread_mutex_t logger_mutex = PTHREAD_MUTEX_INITIALIZER;

const char *logger_str(enum logger_level level) {
    switch (level) {
    case INFO:
        return STR_INFO;
    case WARN:
        return STR_WARN;
    case ERROR:
        return STR_ERR;
    default:
        return "";
    }
}

void logger_log(enum logger_level level, const char *module, const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    pthread_mutex_lock(&logger_mutex);
    fprintf(stderr, "%s[%s] ", logger_str(level), module);
    vfprintf(stderr, fmt, args);
    fprintf(stderr, "\n");
    pthread_mutex_unlock(&logger_mutex);
    va_end(args);
}
