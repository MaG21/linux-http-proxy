#include "http.h"

#define MAX_BUF          4096   /* 4kb! o_o */

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
	int      ret;
	char     *ptr;
	char     *header;
	char     buf[MAX_BUF+1];
	size_t   len;

	ret = recv(sock, &buf[0], MAX_BUF, MSG_PEEK);
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
	memcpy(&header[0], &buf[0], len);
	header[len]='\0';

	ret = recv(sock, &buf[0], len + 4, 0); /* consume. 4 = \r\n\r\n */
	if(ret<0)
		return NULL;

   return header;
}

static struct http_url
http_parse_resource(char *resouce)
{
	char            *token;
	char            *saveptr;
	struct http_url *res;

	token = strtok_r(resource, ":", &saveptr);
	if(!token)
		return NULL;

	res       = calloc(1, sizeof(*res));
	res->host = strdup(token);

	token = strtok_r(NULL, "\0")
	if(!token)
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
	int                 tmp;
	char                *tmp_ptr   = strdup(url);
	char                *token;
	char                *saveptr;
	struct http_url     *URL;
	struct net_resource *res;

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

	URL->host    = res->host
	URL->service = res->service
	if(res->service && !strcmp(token, "443"))
		URL->issecure = TRUE;
	free(res); /* yes, just the structure */

	token = strtok_r(NULL, "\0", &saveptr);
	if(!token)
		goto error;

	tmp = strlen(token);
	URL->path = malloc(sizeof(char)*(tmp+2));
	URL->path[0] = '/';
	memcpy(&URL->path[1], token, tmp);
	URL->path[tmp] = '\0';

	free(tmp_ptr);
	return URL;

error:
	free(tmp_ptr);
	free_http_url(URL);
	return NULL;
}

struct http_request*
http_parse_request(char *header)
{
	char                *tmp = strdup(header);
	char                *token;
	char                *saveptr;
	char                *saveptr2;
	long int            n;
	struct http_request *req;

	token = strtok_r(tmp, HTTP_HEADER_FIELD_SEPARATOR, &saveptr);
	token = strtok_r(token, " ", &saveptr2);
	if(!token) {
		free(tmp);
		return NULL
	}

	req = malloc(sizeof(*req));
	if(!strcasecmp(req->method, "CONNECT")) {
		req->method = strdup("GET");
		req->url = http_parse_resource(token);
	} else {
		req->method = strdup(token);
		req->url = http_parse_url(token);
	}

	token = strtok_r(NULL, " ", &saveptr2);
	if(!token)
		goto error;

	if(!req->url)
		goto error;

	token = strtok_r(NULL, "\0", &saveptr2);
	if(!token)
		goto error;
	req->protocol = strdup(token);

	req->content_lengh = -1;
	while(token = strtok_r(NULL, HTTP_HEADER_FIELD_SEPARATOR, &saveptr)) {
		token = strtok_r(token, ": ", &saveptr2);
		if(strcasecmp(token, "content-length"))
			continue;

		token = strtok_r(token, ": ", &saveptr2);
		if(!token)
			goto error;

		n = utils_parse_number(token);
		if(!errno)
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
 * 	-1 or 0 on error. If -1 is returned errno is set appropriately.
 * 	1 on success.
 */
int
http_proxy_make_request(struct net_proxy *proxy)
{
	int    ret;
	char   *buf;
	char   *header;
	char   *ptr;
	size_t len;
	struct http_request *req;

	header = http_get_header(proxy->client);
	if(!header)
		return -1;

	req = http_parse_request(header);
	if(!req)
		return 1; /* errno is not set */

	ret = net_connect(req->host, req->service);
	if(ret<0)
		return -1;

	proxy->server = ret;

	/* 2 white spaces + 1 CR + 1 LF */
	len = strlen(req->method) + strlen(req->path) +strlen(req->protocol)+4;
	buf = malloc(sizeof(char)*len);
	snprintf(&buf[0], len, "%s %s %s%s", req->method, req->path,
	                          req->protocol, HTTP_HEADER_FIELD_SEPARATOR);

	ret = net_send(proxy->server, &buf[0], len);
	free(buf);
	if(ret<0)
		goto end_func;

	ptr = strstr(header, HTTP_HEADER_FIELD_SEPARATOR);
	if(!ptr)
		goto end_func;

	ret = net_send(proxy->server, ptr, strlen(ptr));
	if(ret<0)
		goto end_func;

	proxy->remaining = req->content_length;
	if(req->content_length!=-1) {
		ret = net_exchange(proxy->client, proxy->server, proxy->remaining);
		if(ret<0)
			error_end_func;
		proxy->remaining -= ret;
		if(!proxy->remaining)
			proxy->remaining = -1;
	}

end_func:
	free(header);
	http_free_request(req);
	return ret;
}


void
http_free_request(struct http_request *req)
{
	if(!req)
		return;
	if(req->method)
		free(req->method);
	if(req->protocol)
		free(req->protocol);

	if(!req->url)
		goto end_func;

	if(req->url->scheme)
		free(req->url->scheme);
	if(req->url->host)
		free(req->url->host);
	if(req->url->path)
		free(req->url->path);
	if(req->url->service)
		free(req->url->service);
	free(req->url);
end_func:
	free(req);
	return;
}

