#ifndef _NET_H
#define _NET_H

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

#endif
