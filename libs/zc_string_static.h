#ifndef ZC_STRING_STATIC_H
#define ZC_STRING_STATIC_H

#include <stdlib.h>
#include <string.h>

#define ZC_API static

ZC_API char *zc_strdup(char const *str)
{
    size_t len = strlen(str) + 1;
    void *new = malloc(len);

    if (new == NULL) return NULL;

    return (char *)memcpy(new, str, len);
}

/*
 * Appends src to string dst of size dsize (unlike strncat, dsize is the
 * full size of dst, not space left).  At most dsize-1 characters
 * will be copied.  Always NUL terminates (unless dsize <= strlen(dst)).
 * Returns strlen(src) + MIN(dsize, strlen(initial dst)).
 * If retval >= dsize, truncation occurred.
 *
 * Copyright (c) 1998, 2015 Todd C. Miller <Todd.Miller@courtesan.com>
 */
ZC_API size_t zc_strlcat(char *dst, char const *src, size_t dsize)
{
    const char *odst = dst;
    const char *osrc = src;
    size_t n = dsize;
    size_t dlen;

    while (n-- != 0 && *dst != '\0')
        dst++;
    dlen = dst - odst;
    n = dsize - dlen;

    if (n-- == 0) return (dlen + strlen(src));
    while (*src != '\0')
    {
        if (n != 0)
        {
            *dst++ = *src;
            n--;
        }
        src++;
    }
    *dst = '\0';

    return (dlen + (src - osrc));
}

/*
 * Copyright (c) 1998, 2015 Todd C. Miller <Todd.Miller@courtesan.com>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */
ZC_API size_t zc_strlcpy(char *dst, char const *src, size_t dsize)
{
    char const *osrc = src;
    size_t nleft = dsize;

    if (nleft != 0)
    {
        while (--nleft != 0)
        {
            if ((*dst++ = *src++) == '\0') break;
        }
    }

    if (nleft == 0)
    {
        if (dsize != 0) *dst = '\0';
        while (*src++)
            ;
    }

    return (size_t)(src - osrc - 1);
}

#endif
