#ifndef _UTILS_H
#define _UTILS_H

#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <getopt.h>
#include <limits.h>

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
}

#endif
