#define _POSIX_C_SOURCE 200809L
#include "exe.h"
#include "die.h"
#include "dirname.h"
#include <stdio.h>
#include <uv.h>

static char path[FILENAME_MAX] = {0};
static char dir[FILENAME_MAX] = {0};
static char cwd[FILENAME_MAX] = {0};

void exe_init(void)
{
    size_t sz = sizeof path;
    if (uv_exepath(path, &sz)) die();

    sz = sizeof dir;
    if (uv_exepath(dir, &sz)) die();
    dirname(dir);

    sz = sizeof cwd;
    if (uv_cwd(cwd, &sz)) die();
}

char const *exe_path(void) { return path; }

char const *exe_dir(void) { return dir; }

char const *exe_cwd(void) { return cwd; }