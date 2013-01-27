#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <unistd.h>
#include <errno.h>
#include <sys/epoll.h>
#include <fcntl.h>

#include "net.h"

/*
 *
 * NOTE:
 *   The compiler (GCC), is smart enough to realise this function should
 *   be inlined. (See: Linux Kernel Condig Style, chapter 15)
 */
static void
net_sys_err(const char *s)
{
	perror(s);
	exit(EXIT_FAILURE);
}

/*
 *
 * NOTE:
 *   The compiler (GCC), is smart enough to realise this function should
 *   be inlined. (See: Linux Kernel Condig Style, chapter 15)
 */
static _Bool
net_check_errno_EAGAIN(void)
{
	return ((errno==EAGAIN) || (errno==EWOULDBLOCK));
}

void
net_set_nonblock(int sock)
{
	int flags;
	int ret;

	ret = fcntl(sock, F_GETFL, 0);
	if(ret<0)
		net_sys_err("fcntl");

	ret = fcntl(sock, F_SETFL, O_NONBLOCK|ret);
	if(ret<0)
		net_sys_err("fcntl");
    return;
}

int
net_epoll_interface_add(int epollfd, int sock, void *dataptr)
{
	struct epoll_event ev;

	ev.events         = EPOLLET | EPOLLIN;
	ev.data.ptr       = dataptr;

	return epoll_ctrl(epollfd, EPOLL_CTL_ADD, sock, &ev);
}

/*
 */
void
net_accept_connections(int epollfd, int sock_accept)
{
	int                sock;
	int                ret;
	struct net_proxy   *proxy;

	while(1) {
		sock = accept(sock_accept, NULL, NULL);
		if(sock<0 && net_check_errno_EAGAIN())
			break;

		if(sock<0)
			continue;

		net_set_nonblock(sock);
		proxy = calloc(1, sizeof(*proxy));

		proxy->fd = sock;

		ret = net_epoll_interface_add(epollfd, sock, proxy);
		if(!ret)
			continue;

		if(errno==ENOSPC) { /* epoll can't watch more users */
			close(proxy->fd);
			free(proxy);
			perror("epoll");
			return;
		}
		net_sys_err("epoll");
	}
	return;
}

void
net_check_sockets(struct epoll_event **evs, size_t n_events)
{
	int   i;

	for(i=0; i < n_events ; i++ ) {
		if((evs[i]->events & EPOLLERR) || (evs[i]->events & EPOLLHUP) ||
		                                 (!(evs[i]->events & EPOLLIN)) ) {
			if(evs[i]->data.ptr->dataptr)
				free(evs[i]->data.ptr->dataptr);

			close(evs[i]->data.ptr->fd);

			if(evs[i]->data.ptr->peer)
				close(evs[i]->data.ptr->peer.fd);

			free(evs[i]->data.ptr);
			evs[i]->data.ptr = NULL;
		}
	}
}

/*
 * service:  this argument can be either a protocol name or a port number.
 *
 * returns:
 * 	This function always success, if not, the program is terminated.
 */
int
net_listen(char *service)
{
	int              sock;
	int              ret;
	struct addrinfo  *rp;
	struct addrinfo  *result;
	struct addrinfo  hints;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family   = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags    = AI_PASSIVE;
	hints.ai_protocol = 0;

	ret = getaddrinfo(NULL, service, &hints, &rp);
	if(ret) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(ret));
		exit(EXIT_FAILURE);
	}

	for(rp=result; rp; rp = rp->ai_next) {
		sock = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
		if(sock<0)
			continue;

		ret = bind(sock, rp->ai_addr, rp->ai_addrlen);
		if(!ret)
			break;

		if(ret<0 && errno==EPERM)
			net_sys_err("bind");

		close(sock);
	}

	if(!rp) {
		fprintf(stderr, "Could not bind\n");
		exit(EXIT_FAILURE);
	}

	freeaddrinf(result);

	/* 128, check the Linux man page for the listen() system call */
	ret = listen(sock, 128);
	if(ret<0)
		net_sys_err("listen");
   return sock;
}

/*
 * returns:
 * 	-1 on error, and errno is set appropriately.
 * 	otherwise a file descriptor that refers to a connected socket.
 */
int
net_connect(const char *host, const char *service)
{
	int               sock;
	int               ret;
	struct addrinfo   hints;
	struct addrinfo   *result;
	struct addrinfo   *rp;

	memset(&hints, 0, sizeof(hints));

	hints.ai_family     = AF_UNSPEC;
	hints.ai_socktype   = SOCK_STREAM;
	hints.ai_flags      = 0;
	hints.ai_protocol   = 0;

	ret = getaddrinfo(host, service, &hints, &result);
	if(ret<0)
		return -1;

	for(rp = result; rp; rp = rp = rp->ai_next) {
		ret = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
		if(ret<0)
			return -1;

		sock = connect(ret, rp->ai_addr, rp->ai_addrlen);
		if(sock!=-1)
			break;
	}

	freeaddrinfo(result);
   return sock;
}

ssize_t
net_send(int sock, const char *buf, size_t nbytes)
{
	size_t       idx  =0;
	ssize_t      n    =0;
	const size_t ret = nbytes;

	do {
		idx += n;
		n = write(sock, &buf[idx], nbytes);
		if(n < 0)
			return -1;

		nbytes -= n;
	} while(nbytes);
   return ret;
}

static int
net_splice(int in, int out, size_t data_len)
{
	int     len;
	size_t  nbytes     =0;

	do {
		len = splice(in, NULL, out, NULL, data_len,
		SPLICE_F_MOVE | SPLICE_F_NONBLOCK);

		if(len <= 0) {
			if(len<0 && net_check_errno_EAGAIN())
				return nbytes;
			else if(!len)
				return nbytes;
			else 
				return -1;
		}
		data_len -= len;
		nbytes   += len;
	} while(data_len);
   return nbytes;
}

/*
 * returns:
 *	-1 on error, and errno is set appropriately.
 *	0 if the file descriptor referrer by sock_in or sock_out is disconnected.
 *	on success, returns the number of bytes transferred.
 */
int
net_exchange(int sock_in, int sock_out, size_t nbytes)
{
	int ret;
	int pipefd[2];

	pipe(pipefd);
	if(ret<0)
		net_sys_err("pipe()");

	ret = net_splice(sock_in, pipefd[1], nbytes);
	if(ret<=0)
		return ret;

	ret = net_splice(pipefd[0], sock_out, ret);
	if(ret<=0)
		return ret;

	close(pipefd[0]);
	close(pipefd[1]);
   return ret;
}

void
net_close_proxy(struct net_proxy *proxy)
{
	if(!proxy)
		return;

	/* protect standard file descriptors */
	if(proxy->fd > 2)
		close(proxy->fd);
	if(proxy->peer.fd > 2 )
		close(proxy->peer.fd);
	free(proxy);
    return;
}

