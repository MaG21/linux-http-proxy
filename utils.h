#ifndef _UTILS_H
#define _UTILS_H

#ifndef TRUE
#define TRUE  0x01
#else
#undef  TRUE
#define TRUE  0x01
#endif

#ifndef FALSE
#define FALSE 0x00
#else
#undef  FALSE
#define FALSE 0x00
#endif

struct utils_options {
	char  *port;
};

/* prototipes */

long int  utils_parse_number(const char *);

struct utils_options*  utils_getopt(int, char* const *);

void  utils_free_options(struct utils_options *);

#endif
