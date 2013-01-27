#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <getopt.h>
#include <limits.h>
#include <stdint.h>

#include "utils.h"

/*
 * returns:
 *      always return a number.
 *      if an error occurs, -1 is returned and errno has a non-zero value.
 *
 * NOTE:
 *     It is very important to check the value of errno once this function
 *     is done.
 */
long int
utils_parse_number(const char *s)
{
	long int   n;	

	errno = 0;
	n = strtol(s, (char**)0, 0x0A);
	if((errno == ERANGE && (n==LONG_MAX || n==LONG_MIN))
	                              || (errno != 0 && n== 0))
		return -1;
	return n;
}

struct util_options*
utils_getopt(int argc, char* const argv[])
{
	char                 c;
	uint8_t              background  = FALSE;
	extern int           opterr;
	extern int           optopt;
	extern char          *optarg;
	struct util_options  *opts;

	opterr = 0;
	opts = malloc(sizeof(*opts));
	opts->port  = strdup("8080");
again:
	c = getopt(argc, argv, ":hdp:b:");
	switch(c) {
	case 'p':
		if(!optarg) {
			fprintf(stderr,
			     "Error: Optior -p has a null argument.\n");
			exit(EXIT_FAILURE);
		}
		free(opts->port);
		opts->port = strdup(optarg);
		goto again;
	case 'd':
		background = TRUE;
		goto again;
	case ':':
		fprintf(stderr, "Option %c requires an operand\n", optopt);
		exit(EXIT_FAILURE);
	case -1:
		break;
	default: /* ? */
		fprintf(stderr, "Usage: %s [-d] [-p port/service]\n", argv[0]);
		exit(EXIT_FAILURE);
	}

	if(background) {
		printf("Going to background...");
		daemon(TRUE, FALSE);
		printf("\tOk\n");
	}
	return opts;
}

void
utils_free_options(struct utils_options *opts)
{
	if(!opts)
		return;
	if(opts->port)
		free(opts->port);
	free(opts);
}

