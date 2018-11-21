#ifndef __NET_H
#define __NET_H

extern char rbuf[512];

int net_connect(const char *host, const char *port, int use_ssl_copy);
int net_recv();
void net_raw(const char *fmt, ...);
void net_raws(char *ptr);
void net_disconnect();

#endif /* __NET_H */
