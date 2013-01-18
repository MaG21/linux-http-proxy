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
	n = strtol(token, (char**)0, 0x0A);
	if((errno == ERANGE && (n==LONG_MAX || val==LONG_MIN))
	                              || (errno != 0 && val == 0))
		return -1;
	return n;
}

struct util_options*
utils_getopt(int argc, const char **argv)
{
	char                 c;
	extern int           opterr  = 0;
	extern int           optopt;
	extern char          *optarg;
	struct util_options  *opts;

	opts = malloc(sizeof(*opts));
	opts->port  = strdup("8080");
again:
	c = getopt(argc, argv, ":hp:b:");
	switch(c) {
	case 'p':
		if(!optarg) {
			fprintf(stderr,
			     "Error: Optior -p has a null argument.\n");
			exit(EXIT_FAILURE);
		}
		opts->port = strdup(optarg);
		goto again;
	case ':':
		fprintf(stderr, "Option %c requires an operand\n", optopt);
		exit(EXIT_FAILURE);
	case -1:
		break;
	default: /* ? */
		fprintf(stderr, "Usage: %s [-p port/service]\n", argv[0]);
		exit(EXIT_FAILURE);
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

void
strip(char *string)
{
	char    *ptr = string;
	size_t  len  = strlen(string);

	while(*ptr) {
		if(isspace(*ptr))
			ptr++;
	}

	if(!*ptr) {
		memset(string, 0, len);
		return;
	} else if(ptr - string) {
		memmove(string, string[ptr-string], len - (ptr-string));
	}

	ptr = string + len - 1;
	if(!isspace(*ptr))
		return;

	while(isspace(*ptr))
		*ptr--;
	*(++ptr) = '\0';
   return;
}

void
strlower(char *string)
{
	char *ptr = string;

	while(*ptr)
		*ptr++ = tolower(*ptr);
  return;
}

