#ifndef _NET_H
#define _NET_H

#include <sys/epoll.h>

/* This program handles non-blocking sockets, please, keep that in mind
 * while reading the source code.
 */
#define NET_SOCKET_CLOSED -1

struct net_proxy {
	struct {
		size_t remaining;
		int    fd;
	} peer;

	size_t remaining;
	int    fd;
};

/* prototipes */

void    net_set_nonblock(int);
int     net_epoll_interface_add(int, struct net_proxy *);
void    net_accept_connections(int, int);
void    net_check_sockets(struct epoll_event *, size_t);
int     net_listen(char *);
int     net_connect(const char *, const char *);
ssize_t net_send(int, const char *, size_t);
int     net_exchange(int, int, size_t);
void    net_close_proxy(struct net_proxy *);

#endif
