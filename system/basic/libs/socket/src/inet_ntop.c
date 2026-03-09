/*
 * Copyright (c) 1996-1999 by Internet Software Consortium.
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND INTERNET SOFTWARE CONSORTIUM DISCLAIMS
 * ALL WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL INTERNET SOFTWARE
 * CONSORTIUM BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL
 * DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR
 * PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS
 * ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
 * SOFTWARE.
 */

#include <sys/param.h>
#include <sys/types.h>
#include <sys/socket.h>

#include <netinet/in.h>
#include <arpa/inet.h>
//#include <arpa/nameser.h>

#include <sys/errno.h>
#include <stdio.h>
#include <string.h>

#ifdef SPRINTF_CHAR
# define SPRINTF(x) strlen(sprintf/**/x)
#else
# define SPRINTF(x) ((size_t)sprintf x)
#endif

static const char *inet_ntop4 (const uint8_t *src, char *dst, socklen_t size);

const char *
inet_ntop (int af, const void *src, char *dst, socklen_t size)
{
	switch (af) {
	case AF_INET:
		return (inet_ntop4(src, dst, size));
	default:
		return (NULL);
	}
}

static void uint_to_str(uint8_t val, char *buf) {
	if (val >= 100) {
		*buf++ = '0' + val / 100;
		val %= 100;
	}
	if (val >= 10) {
		*buf++ = '0' + val / 10;
		val %= 10;
	}
	*buf++ = '0' + val;
	*buf = '\0';
}

static const char *
inet_ntop4 (const uint8_t *src, char *dst, socklen_t size)
{
	char tmp[sizeof "255.255.255.255"];
	char *ptr = tmp;
	char octet_str[4];
	
	// Convert first octet
	uint_to_str(src[0], octet_str);
	strcpy(ptr, octet_str);
	ptr += strlen(octet_str);
	
	// Add dot and second octet
	*ptr++ = '.';
	uint_to_str(src[1], octet_str);
	strcpy(ptr, octet_str);
	ptr += strlen(octet_str);
	
	// Add dot and third octet
	*ptr++ = '.';
	uint_to_str(src[2], octet_str);
	strcpy(ptr, octet_str);
	ptr += strlen(octet_str);
	
	// Add dot and fourth octet
	*ptr++ = '.';
	uint_to_str(src[3], octet_str);
	strcpy(ptr, octet_str);
	ptr += strlen(octet_str);
	
	// Null terminate
	*ptr = '\0';

	if (ptr - tmp >= size) {
		return (NULL);
	}
	return strcpy(dst, tmp);
}
