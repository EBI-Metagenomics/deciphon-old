#include "exe.h"
#include "die.h"
#include "dirname.h"
#include <uv.h>

char const *exe_path(void)
{
    static char path[FILENAME_MAX] = {0};
    size_t sz = sizeof path;
    if (uv_exepath(path, &sz)) die();
    return path;
}

char const *exe_dir(void)
{
    static char dir[FILENAME_MAX] = {0};
    size_t sz = sizeof dir;
    if (uv_exepath(dir, &sz)) die();
    dirname_musl(dir);
    return dir;
}
