#include "is.h"
#include "zc.h"
#include <errno.h>

bool is_int(char const *str)
{
    zc_strto_int(str, NULL, 10);
    bool ok = !errno;
    errno = 0;
    return ok;
}

bool is_long(char const *str)
{
    zc_strto_long(str, NULL, 10);
    bool ok = !errno;
    errno = 0;
    return ok;
}

bool is_int32(char const *str)
{
    zc_strto_int32(str, NULL, 10);
    bool ok = !errno;
    errno = 0;
    return ok;
}

bool is_int64(char const *str)
{
    zc_strto_int64(str, NULL, 10);
    bool ok = !errno;
    errno = 0;
    return ok;
}
