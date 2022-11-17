#include "core/exe.h"
#include "core/die.h"
#include "core/dirname.h"
#include <stdio.h>
#include <uv.h>

static char path[FILENAME_MAX] = {0};
static char dir[FILENAME_MAX] = {0};

void exe_init(void)
{
    size_t sz = sizeof path;
    if (uv_exepath(path, &sz)) die();

    sz = sizeof dir;
    if (uv_exepath(dir, &sz)) die();
    dirname(dir);
}

char const *exe_path(void) { return path; }
char const *exe_dir(void) { return dir; }
