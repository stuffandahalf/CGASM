#include <ctype.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "lang.h"

int
printef(const char *fmt, ...)
{
	int c;
	va_list args;
	va_start(args, fmt);

	c = vfprintf(stderr, fmt, args);

	va_end(args);

	return c;
}

long
fsize(FILE *fptr)
{
	long current_pos;
	long size;
	
	current_pos = ftell(fptr);

	fseek(fptr, 0, SEEK_END);
	size = ftell(fptr);
	fseek(fptr, current_pos, SEEK_SET);

	return size;
}

char *
str_clone(const char *src)
{
	char *new_ptr = malloc(sizeof(char) * (strlen(src) + 1));
	if (new_ptr == NULL) {
		return NULL;
	}
	strcpy(new_ptr, src);
	return new_ptr;
}

char *
str_to_upper(char *str)
{
	char *c = NULL;
	for (c = str; *c != '\0'; c++) {
		*c = (char)toupper(*c);
	}
	return str;
}

char *
str_trim(char *str)
{
	int done = 0;
	char *c;
	char *end;

	while (*str == ' ' || *str == '\t') str++;  /* find first non whitespace character */

	end = str;  /* starting from the first whitespace character: */
	while (!done) {
		while (*end != ' ' && *end != '\t' && *end != '\0') {   /* check if the character is not a whitespace character */
			end++;
		}

		if (*end == '\0') { /* if the current character is the end of the string */
			return str;
		}

		for (c = end; *c == '\t' || *c == ' '; c++);	/* look for the next non-whitespace character; */

		if (*c == '\0') {
			*end = '\0';
			done = 1;
		} else {
			end = c;
		}
	}

	return str;
}

