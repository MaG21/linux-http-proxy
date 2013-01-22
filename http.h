#ifndef _HTTP_
#define _HTTP_
#include <sys/types.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <limits.h>
#include <errno.h>

#define TRUE               0x01
#define FALSE              0x00

#define CR                 "\r"
#define LF                 "\n"
#define HTTP_HEADER_FIELD_SEPARATOR    CR##LF
#define HTTP_HEADER_END_OF_HEADER      CR##LF##CR##LF

#define HTTP_SCHEME_LENGTH             0x06

struct http_url {
	char      *scheme;   /* either HTTP or HTTPS */
	char      *host;
	char      *path;
	char      *port;
	uint8_t   issecure;  /* TRUE or FALSE */
};

struct http_request {
	struct http_url  *url;
	char             *method;
	char             *protocol;
	ssize_t          content_length;
};

#endif
