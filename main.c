#include "http.h"
#include "net.h"
#include "utils.h"

#define MAX_EVENTS 128

int
main(int argc, char **argv)
{
	int                   i;
	int                   n
	int                   ret;
	int                   sock;
	int                   epollfd;
	struct epoll_event    *events;
	struct  utils_options *opts;	

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
				net_accept_connections(sock, epollfd);
				continue;
			}

			/* proxy */
		}
	}
