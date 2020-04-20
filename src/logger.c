#include <stdio.h>
#include <stdarg.h>
#include <pthread.h>
#include <time.h>
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
    char timestamp_buf[32];
    time_t ctime;
    struct tm *tm;
    va_list args;

    /* get the current time */
    ctime = time(NULL);
    tm = localtime(&ctime);
    sprintf(timestamp_buf, "%04d-%02d-%02d %02d:%02d:%02d", tm->tm_year+1900,
            tm->tm_mon+1, tm->tm_mday, tm->tm_hour, tm->tm_min, tm->tm_sec);

    /* print rest of the stuff */
    va_start(args, fmt);
    pthread_mutex_lock(&logger_mutex);
    fprintf(stderr, "[%s]%s[%s] ", timestamp_buf, logger_str(level), module);
    vfprintf(stderr, fmt, args);
    fprintf(stderr, "\n");
    pthread_mutex_unlock(&logger_mutex);
    va_end(args);
}
