#ifndef __LOGGER_H
#define __LOGGER_H

enum logger_level {
    INFO,
    WARN,
    ERROR
};

void logger_log(enum logger_level level, const char *module, const char *fmt, ...);

#endif /* __LOGGER_H */
