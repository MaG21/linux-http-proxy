#ifndef _NET_H
#define _NET_H
#include <netdb.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <unistd.h>
#include <errno.h>

#define NET_SOCKET_CLOSED -1

/* This program handles non-blocking sockets, please, keep that in mind
 * while reading the source code.
 */

struct net_proxy {
	size_t  data_len;
	int     client;
	int     server;
	char    *dataptr;
}
#endif
