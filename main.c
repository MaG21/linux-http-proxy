/*
 * FIXME: Identation levels too deep.
 */
#include <sys/epoll.h>
#include <stdlib.h>
#include <stdio.h>

#include "http.h"
#include "net.h"
#include "utils.h"

#define MAX_EVENTS 128

int
main(int argc, char **argv)
{
	int                  i;
	int                  n;
	int                  ret;
	int                  sock;
	int                  epollfd;
	struct net_proxy     *proxy;
	struct epoll_event   *events;
	struct utils_options *opts;

	opts = utils_getopt(argc, argv);

	sock = net_listen(opts->port);
	utils_free_options(opts);
	net_set_nonblock(sock);

	epollfd = epoll_create1(0);
	if(epollfd<0) {
		perror("epoll");
		exit(EXIT_FAILURE);
	}

	events = calloc(MAX_EVENTS, sizeof(*events));

	for( ; ; ) {
		ret = epoll_wait(epollfd, events, MAX_EVENTS, -1);
		if(ret<0) {
			perror("epoll");
			exit(EXIT_FAILURE);
		}

		net_check_sockets(events, ret);

		for(i=0, n=ret; i < n; i++) {
			if(!events[i].data.ptr)
				continue;

			if(events[i].data.ptr->client==sock) {
				net_accept_connections(epollfd, sock);
				continue;
			}

			/* proxy */
			if(events[i].data.ptr->remaining) {
				proxy = events[i].data.ptr;
				ret = -1;

				if(proxy->peer->remaining)
					net_close_proxy(proxy);

				ret = net_exchange(proxy->fd, proxy->peer.fd, proxy-remaining);

				if(ret<=0)
					net_close_proxy(proxy);
				continue;
			}

			ret = http_proxy_make_request(epollfd, events[i]->data.ptr);
			switch(ret) {
			case -1:
				perror("http_proxy_make_request");
			case 1:
				net_close_proxy(events[i]->data.ptr);
			}

			/* The server must be watched in another epoll instance */
			proxy = calloc(1, sizeof(*proxy));

			proxy->fd             = events[i]->peer.fd;
			proxy->remaining      = events[i]->peer.remaining;
			proxy->peer.fd        = events[i]->fd;
			proxy->peer.remaining = events[i]->remaining;
			net_epoll_interface_add(epollfd, proxy);
		}
	}
   return EXIT_SUCCESS;
}

