#ifndef _HTTP_
#define _HTTP_

#include "net.h"

#define HTTP_HEADER_END_OF_HEADER      "\r\n\r\n"
#define HTTP_HEADER_FIELD_SEPARATOR    "\r\n"

#define HTTP_PROXY_ERROR
#define HTTP_PROXY_CLOSE_CONNECTION

struct http_url {
	char      *scheme;   /* either HTTP or HTTPS */
	char      *host;
	char      *path;
	char      *service;
	char      issecure;  /* TRUE or FALSE */
};

struct http_request {
	struct http_url *url;
	char            *method;
	char            *protocol;
	size_t          content_length;
};

/* prototipes */

int http_proxy_make_request(int , struct net_proxy *);

#endif
