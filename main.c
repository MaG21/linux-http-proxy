/*
 * FIXME: Identation level
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
	struct net_proxy     *newproxy;
	struct epoll_event   *events;
	struct utils_options *opts;

	opts = utils_getopt(argc, argv);

	printf("Init server on port %s ...", opts->port);
	fflush(stdout);

	sock = net_listen(opts->port);
	utils_free_options(opts);
	net_set_nonblock(sock);

	epollfd = epoll_create1(0);
	if(epollfd<0) {
		perror("epoll");
		exit(EXIT_FAILURE);
	}

	proxy = malloc(sizeof(*proxy));
	proxy->fd = sock;
	net_epoll_interface_add(epollfd, proxy);

	puts("Ok");

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

			proxy = events[i].data.ptr;

			if(proxy->fd==sock) {
				net_accept_connections(epollfd, sock);
				continue;
			}

			/* proxy */
			if(proxy->remaining) {
				if(proxy->peer.remaining)
					net_close_proxy(proxy);

				ret = net_exchange(proxy->fd, proxy->peer.fd, proxy->remaining);

				if(ret<=0)
					net_close_proxy(proxy);
				continue;
			}

			ret = http_proxy_make_request(epollfd, proxy);
			switch(ret) {
			case -1:
				perror("http_proxy_make_request");
			case 1:
				net_close_proxy(proxy);
				continue;
			}

			/* The server must be watched in another epoll instance */
			newproxy = calloc(1, sizeof(*proxy));

			newproxy->fd             = proxy->peer.fd;
			newproxy->remaining      = proxy->peer.remaining;
			newproxy->peer.fd        = proxy->fd;
			newproxy->peer.remaining = proxy->remaining;
			net_epoll_interface_add(epollfd, proxy);
		}
	}
   return EXIT_SUCCESS;
}

