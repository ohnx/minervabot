#include <stdio.h>
#include <stdarg.h>
#include <unistd.h>
#include <string.h>
#include <netdb.h>
#include <errno.h>
#include "mbedtls/net.h"
#include "mbedtls/ssl.h"
#include "mbedtls/entropy.h"
#include "mbedtls/ctr_drbg.h"
#include "mbedtls/debug.h"
#include "logger.h"
#include "net.h"

/* regular stuff */
int conn;
char sbuf[512];
int use_ssl;

/* ssl stuff */
mbedtls_net_context server_fd;
mbedtls_entropy_context entropy;
mbedtls_ctr_drbg_context ctr_drbg;
mbedtls_ssl_context ssl;
mbedtls_ssl_config conf;

int net_connect_nossl(const char *host, const char *port) {
    struct addrinfo hints, *res;
    int ret;

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    if ((ret = getaddrinfo(host, port, &hints, &res))) {
        logger_log(ERROR, "net", "failed to resolve server");
        return ret;
    }

    conn = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (conn == -1) {
        logger_log(ERROR, "net", "failed to create socket");
        freeaddrinfo(res);
        return errno;
    }

    if ((ret = connect(conn, res->ai_addr, res->ai_addrlen))) {
        logger_log(ERROR, "net", "failed to connect to server");
        freeaddrinfo(res);
        return ret;
    }

    freeaddrinfo(res);

    return 0;
}

int net_connect_ssl(const char *host, const char *port) {
    int ret;

    /* initialize all the storage structures */
    mbedtls_net_init(&server_fd);
    mbedtls_ssl_init(&ssl);
    mbedtls_ssl_config_init(&conf);
    mbedtls_ctr_drbg_init(&ctr_drbg);

    /* initialize entropy */
    mbedtls_entropy_init(&entropy);
    if ((ret = mbedtls_ctr_drbg_seed(&ctr_drbg, mbedtls_entropy_func, &entropy,
                               (const unsigned char *)host, strlen(host)))) {
        /* failed to init entropy */
        logger_log(ERROR, "net", "failed to init entropy: mbedtls_ctr_drbg_seed returned %d", ret);
        return -__LINE__;
    }

    /* start connection */
    if ((ret = mbedtls_net_connect(&server_fd, host, port, MBEDTLS_NET_PROTO_TCP))) {
        logger_log(ERROR, "net", "failed to connect to server: mbedtls_net_connect returned %d", ret);
        return -__LINE__;
    }

    /* more setup stuff */
    if ((ret = mbedtls_ssl_config_defaults(&conf, MBEDTLS_SSL_IS_CLIENT, MBEDTLS_SSL_TRANSPORT_STREAM, MBEDTLS_SSL_PRESET_DEFAULT))) {
        logger_log(ERROR, "net", "failed to configure ssl: mbedtls_ssl_config_defaults returned %d", ret);
        return -__LINE__;
    }

    mbedtls_ssl_conf_authmode(&conf, MBEDTLS_SSL_VERIFY_NONE);
    mbedtls_ssl_conf_rng(&conf, mbedtls_ctr_drbg_random, &ctr_drbg);

    if ((ret = mbedtls_ssl_setup(&ssl, &conf)) != 0) {
        logger_log(ERROR, "net", "failed to configure ssl: mbedtls_ssl_setup returned %d", ret);
        return -__LINE__;
    }

    if((ret = mbedtls_ssl_set_hostname(&ssl, host))) {
        logger_log(ERROR, "net", "failed to configure ssl: mbedtls_ssl_set_hostname returned %d", ret);
        return -__LINE__;
    }

    mbedtls_ssl_set_bio(&ssl, &server_fd, mbedtls_net_send, mbedtls_net_recv, NULL);

    /* begin handshake attempt */
    while ((ret = mbedtls_ssl_handshake(&ssl))) {
        if (ret != MBEDTLS_ERR_SSL_WANT_READ && ret != MBEDTLS_ERR_SSL_WANT_WRITE) {
            logger_log(ERROR, "net", "failed to connect to server: mbedtls_ssl_handshake returned -0x%x", ret);
            return -__LINE__;
        }
    }

    return 0;
}

int net_connect(const char *host, const char *port, int use_ssl_copy) {
    /* check if we use ssl */
    use_ssl = use_ssl_copy;

    logger_log(INFO, "net", "attempting to connect to %s:%s %s", host, port, use_ssl?"using SSL":"without SSL");

    /* call the appropriate initializer */
    if (use_ssl) return net_connect_ssl(host, port);
    else return net_connect_nossl(host, port);
}

int net_recv() {
    static int c;
    if (use_ssl) c = mbedtls_ssl_read(&ssl, (unsigned char *)sbuf, 512);
    else c = read(conn, sbuf, 512);

    if (c == -1) {
        switch (errno) {
        case ECONNRESET:
        case ENOTCONN:
        case ETIMEDOUT:
            logger_log(ERROR, "net", "connection closed by server");
            break;
        }
    }

    return c;
}


void net_raw(const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(sbuf, 512, fmt, ap);
    va_end(ap);
    printf("<< %s", sbuf);
    if (use_ssl) mbedtls_ssl_write(&ssl, (const unsigned char *)sbuf, strlen(sbuf));
    else write(conn, sbuf, strlen(sbuf));
}

void net_raws(char *ptr) {
    printf("<< %s", ptr);
    if (use_ssl) mbedtls_ssl_write(&ssl, (const unsigned char *)sbuf, strlen(sbuf));
    else write(conn, sbuf, strlen(sbuf));
}

void net_disconnect() {
    if (!use_ssl) {
        close(conn);
        return;
    }

    mbedtls_net_free(&server_fd);
    mbedtls_ssl_free(&ssl);
    mbedtls_ssl_config_free(&conf);
    mbedtls_ctr_drbg_free(&ctr_drbg);
    mbedtls_entropy_free(&entropy);
}

