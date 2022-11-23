#include "die.h"
#include <stddef.h>
#include <uv.h>

static char path[FILENAME_MAX] = {0};

char const *cwd(void)
{
    size_t sz = sizeof path;
    if (uv_cwd(path, &sz)) die();
    return path;
}
