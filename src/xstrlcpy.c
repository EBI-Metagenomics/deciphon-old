#include "xstrlcpy.h"
#include <assert.h>
#include <stdio.h>

size_t xstrlcpy(char *restrict dst, char const *restrict src, size_t dst_size)
{
    /* Besides quibbles over the return type (size_t versus int) and signal
     * handler safety (snprintf(3) is not entirely safe on some systems), the
     * following two are equivalent:
     *
     *       n = strlcpy(dst, src, dst_size);
     *       n = snprintf(dst, dst_size, "%s", src);
     *
     * Like snprintf(3), the strlcpy() and strlcat() functions return the total
     * length of the string they tried to create.  For strlcpy() that means the
     * length of src.  For strlcat() that means the initial length of dst plus
     * the length of src.
     *
     * If the return value is >= dst_size, the output string has been truncated.
     * It is the caller's responsibility to handle this. */
    int n = snprintf(dst, dst_size, "%s", src);
    assert(n >= 0);
    return (size_t)n;
}
