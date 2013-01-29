#include <sys/types.h>
#include <sys/socket.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <errno.h>

#include "utils.h"
#include "net.h"
#include "http.h"

#define MAX_BUF          4096   /* 4kb! o_o */

static void
http_free_url(struct http_url *url)
{
	if(!url)
		return;

	if(url->scheme)
		free(url->scheme);
	if(url->host)
		free(url->host);
	if(url->path)
		free(url->path);
	if(url->service)
		free(url->service);
	free(url);
	return;
}

static void
http_free_request(struct http_request *req)
{
	if(!req)
		return;
	if(req->method)
		free(req->method);
	if(req->protocol)
		free(req->protocol);

	http_free_url(req->url);
	return;
}

/*
 * returns:
 *      NULL on error and errno is set appropriately.
 *      a NUL terminated string.
 *
 * NOTE:
 *    The value returned by this function must be freed
 *    once done.
 */
static char*
http_get_header(int sock)
{
	int    ret;
	char   *ptr;
	char   *header;
	char   buf[MAX_BUF+1];
	size_t len;

	ret = net_recv(sock, &buf[0], MAX_BUF, MSG_PEEK);
	if(ret<0)
		return NULL;

	buf[ret]='\0';
	ptr = strstr(&buf[0], HTTP_HEADER_END_OF_HEADER);
	if(!ptr) {
		errno = ECONNABORTED;
		return NULL;
	}

	len = ptr - buf;
	header = malloc(sizeof(char)*(len+1));

	ret = net_recv(sock, &header[0], len, 0);
	if(ret<0)
		return NULL;

	header[ret]='\0';
   return header;
}
 
/*
 * returns:
 *      NULL if the resource could not be parsed.
 *      NON-NULL.
 *
 * NOTE:
 *    The value returned by this function must be freed
 *    once done.
 */
static struct http_url*
http_parse_resource(char *resource)
{
	char            *token;
	char            *saveptr;
	struct http_url *res;

	token = strtok_r(resource, ":", &saveptr);
	if(!token)
		return NULL;

	res       = calloc(1, sizeof(*res));
	res->host = strdup(token);

	token = strtok_r(NULL, "\0", &saveptr);
	if(token)
		res->service = strdup(token);
	else
		res->service = strdup("80"); /* by default 80 */
   return res;
}

/*
 * returns:
 *      NULL if the url could not be parsed.
 * 	NON-NULL.
 *
 * NOTE:
 *    The value returned by this function must be freed
 *    once done.
 */ 	
static struct http_url*
http_parse_url(char *url)
{
	int             tmp;
	char            *tmp_ptr   = strdup(url);
	char            *token;
	char            *saveptr;
	struct http_url *URL;
	struct http_url *res;

	token = strtok_r(tmp_ptr, "://", &saveptr);
	if(strcasecmp(token, "http") && strcasecmp(token, "https")) {
		free(tmp_ptr);
		return NULL;
	}

	URL           = malloc(sizeof(*URL));
	URL->scheme   = strdup(token);
	URL->issecure = !strcasecmp(token, "https") ? TRUE : FALSE;

	token = strtok_r(NULL, "/", &saveptr);
	if(!token)
		goto error;

	res = http_parse_resource(token);
	if(!res)
		goto error;

	URL->host    = res->host;
	URL->service = res->service;
	if(res->service && !strcmp(token, "443"))
		URL->issecure = TRUE;
	free(res); /* yes, just the structure */

	token = strtok_r(NULL, "\0", &saveptr);
	if(!token) {
		URL->path = strdup("/");
	} else {
		tmp = strlen(token);
		URL->path = malloc(sizeof(char)*(tmp+2));
		URL->path[0] = '/';
		memcpy(&URL->path[1], token, tmp);
		URL->path[tmp] = '\0';
	}

	free(tmp_ptr);
	return URL;

error:
	free(tmp_ptr);
	http_free_url(URL);
	return NULL;
}

static struct http_request*
http_parse_request(char *header)
{
	char                *tmp      = strdup(header);
	char                *token;
	char                *ptr;
	char                *saveptr;
	char                *saveptr2;
	long int            n;
	struct http_request *req;

	token = strtok_r(tmp, HTTP_HEADER_FIELD_SEPARATOR, &saveptr);
	token = strtok_r(token, " ", &saveptr2);
	if(!token) {
		free(tmp);
		return NULL;
	}

	ptr = token;

	token = strtok_r(NULL, " ", &saveptr2);
	if(!token)
		goto error;

	req = malloc(sizeof(*req));
	if(!strcasecmp(ptr,  "CONNECT")) {
		req->method = strdup("GET");
		req->url    = http_parse_resource(token);
	} else {
		req->method = strdup(ptr);
		req->url    = http_parse_url(token);
	}

	if(!req->url)
		goto error;

	token = strtok_r(NULL, "\0", &saveptr2);
	if(!token)
		goto error;
	req->protocol = strdup(token);

	req->content_length = 0;
	while((token=strtok_r(NULL, HTTP_HEADER_FIELD_SEPARATOR, &saveptr))) {
		token = strtok_r(token, ": ", &saveptr2);
		if(strcasecmp(token, "content-length"))
			continue;

		token = strtok_r(NULL, "\0", &saveptr2);
		if(!token)
			goto error;

		n = utils_parse_number(token);
		if(errno || n<0) /* content-length must be positive */
			goto error;

		req->content_length = n;
		break;
	}
	free(tmp);
	return req;
error:
	free(tmp);
	http_free_request(req);
	return NULL;
}

/*
 * returns
 * 	-1 or 1 on error. If -1 is returned errno is set appropriately.
 * 	0 on success.
 */
int
http_proxy_make_request(int epollfd, struct net_proxy *proxy)
{
	int    ret;
	char   *header;
	char   *ptr;
	struct http_request *req;

	header = http_get_header(proxy->fd);
	if(!header)
		return -1;

	req = http_parse_request(header);
	if(!req) {
		free(header);
		return 1;
	}

	ret = net_connect(req->url->host, req->url->service);
	if(ret<0)
		goto end_func;

	net_set_nonblock(ret);
	proxy->peer.fd = ret;

	ret=dprintf(proxy->peer.fd, "%s %s %s%s", req->method, req->url->path,
	                           req->protocol, HTTP_HEADER_FIELD_SEPARATOR);
	if(ret<0)
		goto end_func;

	ptr = strstr(header, HTTP_HEADER_FIELD_SEPARATOR);
	if(!ptr)
		goto end_func;

	ret = net_send(proxy->peer.fd, ptr, strlen(ptr));
	if(ret<0)
		goto end_func;

	proxy->remaining = req->content_length;
	if(proxy->remaining) {
		ret =net_exchange(proxy->fd, proxy->peer.fd, proxy->remaining);
		if(ret<=0) {
			net_close_proxy(proxy);
			goto end_func;
		}
		proxy->remaining -= ret;
	}

end_func:
	free(header);
	http_free_request(req);
	return ret;
}

