#include <sys/types.h>
#include <sys/socket.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "utils.h"
#include "net.h"
#include "http.h"


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
	URL->issecure = !strcasecmp(token, "https");

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

#define MAX_BUF     4096   /* 4kb! o_o */
#define MAX_HEADER  100    /* apache's limit */

static struct http_request*
http_parse_request(int sock)
{

	int         minor_version;
	char        buf[MAX_BUF+1];
	size_t      path_len;
	size_t      method_len;
	size_t      nbytes           = 0;
	size_t      num_headers;
	ssize_t     ret;
	const char* method;
	const char* path;

	struct phr_header   headers[MAX_HEADER];
	struct http_request *req;

	while(42) {
		if(nbytes >= MAX_BUF)
			return NULL;   /* the header is too long */

		ret = net_recv(sock, &buf[ret], MAX_BUF, 0);

		if(ret<=0)
			return NULL;

		nbytes     += ret;
		buf[nbytes] = '\0';

		num_headers = MAX_HEADER;
		ret = phr_parse_request(&buf[0], nbytes, &method, &method_len,
		                             &path, &path_len, &minor_version,
					     headers, &num_headers, 0);
		if(ret>0)
			break;
		else if(ret == -1)
			return NULL;

		/* keep receiving information */
	}

	req = calloc(1, sizeof(*req));

	if(ret < nbytes) {
		req->remaining        = strdup(&buf[nbytes]);
		req->remaining_length = nbytes - ret;
	}

	if(!strcasecmp(method,  "CONNECT")) {
		req->method = strdup("GET");
		req->url    = http_parse_resource(path);
	} else {
		req->method = strdup(method);
		req->url    = http_parse_url(path);
	}

	req->protocol = strdup(minor_version);

	ret = utils_parse_number(ptr);
	if(errno || ret<0) { /* content-length must be positive */
		http_free_request(req);
		return NULL;
	}

	req->content_length = n;

	errno = 0;
	return req;
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
	char   *ptr;
	struct http_request *req;

	req = http_parse_request(proxy->fd);
	if(!req) {
		free(header);
		return errno ? -1 : 1;
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

