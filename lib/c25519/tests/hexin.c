#include <stdio.h>
#include <stdlib.h>
#include "hexin.h"

static unsigned int hex1(int c) {
	int v = 0;
	if ('0' <= c && c <= '9') v = c - '0';
	else if ('a' <= c && c <= 'f') v = c - 'a' + 10;
	else if ('A' <= c && c <= 'F') v = c - 'A' + 10;
	else printf("Bad hex: %c\n", c);
	return v;
}

ssize_t get_from_hex(unsigned char *buff, size_t buflen, int delim)
{
	char *lineptr = NULL;
	size_t n = 0;
	ssize_t ll = getdelim(&lineptr, &n, delim, stdin);
	if (ll<0) return ll;
	if (!buff) return -1;
	/* printf("ll %ld  delim %d\n", ll, delim); */
	if (!lineptr || ll<1) return -1;
	if (lineptr[ll-1] == delim) ll-=1;
	if (ll & 0x1) return -1;  /* need even number of ASCII hex */
	if (buflen > (unsigned)ll/2) buflen = ll/2;
	for (size_t ix = 0; ix < buflen; ix++) {
		buff[ix] = hex1(lineptr[ix*2])<<4 | hex1(lineptr[ix*2+1]);
	}
	free(lineptr);
	return buflen;
}

