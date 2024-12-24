#include <stdio.h>

#include "getopt.h"

/*static unsigned int optcharind = 1;*/
char *optarg = NULL;
int optind = 1;
int opterr = 1;
int optopt;
static unsigned int nextchar = 0;

/*#define GASMIC_POSIX_COMPLIANT 1*/

int
getopt(int argc, char *const argv[], const char *arglist)
{
	const char *cptr;
	char cret;

#ifndef GASMIC_POSIX_COMPLIANT
	if (arglist == NULL) {		/* no argument list */
		return -1;
	}
#endif
	if (arglist[0] == '\0') {	/* empty argument list */
		return -1;
	}

	if (optind >= argc) {		/* argument out of range */
		return -1;
	}

	if (nextchar == 0) {		/* if this is the first character of the argument, skip apostrophes */
		if (argv[optind][nextchar] == '-') {
			nextchar++;
			if (argv[optind][nextchar] == '-') {	/* -- argument ends parsing immediately */
				optarg = NULL;
				nextchar = 0;
				return -1;
			}
		} else {
			if (arglist[0] == '+') {	/* in unix, invalid argument */
				optarg = NULL;
				nextchar = 0;
				return -1;
			} else if (arglist[0] == '-') {		/* return argument as for character 1 */
				optarg = argv[optind];
				optind++;
				nextchar = 0;
				return 1;
			}
		}
	}

	if (argv[optind][nextchar] == '\0') {
		if (arglist[0] == '-') {		/* a single minus can be an argument */
			optarg = argv[optind];
			optind++;
			nextchar = 0;
			return 1;
		} else {
			nextchar = 0;
			optarg = NULL;
			return -1;
		}
	}

	cptr = arglist;
	while (*cptr != '\0') {
		if (*cptr == ':') {
			cptr++;
			continue;
		}

		if (*cptr == argv[optind][nextchar]) {
			cret = argv[optind][nextchar];
			cptr++;
			if (*cptr == ':') {			/* takes argument */
				cptr++;
				/* TODO: These two branches only differ slightly, need to refactor */
				if (*cptr == ':') {		/* optional argument */
					nextchar++;
					if (argv[optind][nextchar] == '\0') {
						optarg = NULL;
					} else {
						optarg = &argv[optind][nextchar];
					}
					optind++;
					nextchar = 0;
				} else {				/* required argument */
					cptr--;
					nextchar++;
					if (argv[optind][nextchar] == '\0') {
						optarg = NULL;
						if (opterr) {
							fprintf(stderr, "%s: option requires an argument -- '%c'\n", argv[0], cret);

							return '?';
						}
					} else {
						optarg = &argv[optind][nextchar];
					}
					optind++;
					nextchar = 0;
				}
			} else {					/* no argument */
				cptr--;
				optarg = NULL;
				nextchar++;
				if (argv[optind][nextchar] == '\0') {
					optind++;
					nextchar = 0;
				}
			}
			return cret;
		}

		cptr++;
	}

	if (opterr) {
		fprintf(stderr, "%s: invalid option -- '%c'\n", argv[0], argv[optind][nextchar]);
	}
	nextchar++;
	if (argv[optind][nextchar] == '\0') {
		optind++;
		nextchar = 0;
		optarg = NULL;
	}
	return '?';
}

int
getopt_long(int argc, char *const argv[],
				const char *optstring,
				const struct option *longopts, int *longindex)
{
	return -1;
}

