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

#include <errno.h>
#include <stdio.h>
#include <string.h>

static int inet_pton4 (const char *src, uint8_t *dst);

int
inet_pton (int af, const char *src, void *dst)
{
    switch (af) {
    case AF_INET:
        return (inet_pton4(src, dst));
    default:
        return (-1);
    }
}

static int
inet_pton4 (const char *src, uint8_t *dst)
{
    static const char digits[] = "0123456789";
    int saw_digit, octets, ch;
    uint8_t tmp[4], *tp;

    saw_digit = 0;
    octets = 0;
    *(tp = tmp) = 0;
    while ((ch = *src++) != '\0') {
        const char *pch;

        if ((pch = strchr(digits, ch)) != NULL) {
            unsigned int new_val = *tp * 10 + (pch - digits);

            if (new_val > 255) {
                return (0);
            }
            *tp = new_val;
            if (!saw_digit) {
                if (++octets > 4) {
                    return (0);
                }
                saw_digit = 1;
            }
        } else if (ch == '.' && saw_digit) {
            if (octets == 4) {
                return (0);
            }
            *++tp = 0;
            saw_digit = 0;
        } else {
            return (0);
        }
    }
    if (octets < 4) {
        return (0);
    }
    memcpy(dst, tmp, 4);
    return (1);
}