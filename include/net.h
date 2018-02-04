#ifndef __NET_H
#define __NET_H

extern char sbuf[512];

void net_raw(const char *fmt, ...);
void net_raws(char *ptr);
void net_connect(const char *host, const char *port);
void net_disconnect();
int net_recv();

#endif /* __NET_H */
