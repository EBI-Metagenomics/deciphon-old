#include "dcp_strlcpy.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>

size_t dcp_strlcpy(char *dst, const char *src, size_t len)
{
    /* Besides quibbles over the return type (size_t versus int) and signal
     * handler safety (snprintf(3) is not entirely safe on some systems), the
     * following two are equivalent:
     *
     *       n = strlcpy(dst, src, len);
     *       n = snprintf(dst, len, "%s", src);
     *
     * Like snprintf(3), the strlcpy() and strlcat() functions return the total
     * length of the string they tried to create.  For strlcpy() that means the
     * length of src.  For strlcat() that means the initial length of dst plus
     * the length of src.
     *
     * If the return value is >= dstsize, the output string has been truncated.
     * It is the caller's responsibility to handle this. */
    int n = snprintf(dst, len, "%s", src);
    assert(n >= 0);
    return (size_t)n;
}
