#include "path.h"
#include "xstrlcpy.h"
#include <string.h>

bool append_ext(char *str, size_t len, size_t max_size, char const *ext);
bool change_ext(char *str, size_t pos, size_t max_size, char const *ext);
char *musl_basename(char *s);

bool path_change_or_add_ext(char str[static 1], size_t max_size,
                            char const ext[static 1])
{
    size_t len = strlen(str);
    if (!change_ext(str, len, max_size, ext))
        return append_ext(str, len, max_size, ext);
    return true;
}

void path_basename(char filename[DCP_FILENAME_SIZE],
                   char const path[DCP_PATH_SIZE])
{
    char tmp[DCP_PATH_SIZE] = {0};
    xstrlcpy(tmp, path, DCP_PATH_SIZE);
    char *p = musl_basename(tmp);
    xstrlcpy(filename, p, DCP_FILENAME_SIZE);
}

bool append_ext(char *str, size_t len, size_t max_size, char const *ext)
{
    char *j = &str[len];
    size_t n = strlen(ext);
    if (n + 1 + (size_t)(j - str) > max_size) return false;
    *(j++) = *(ext++);
    *(j++) = *(ext++);
    *(j++) = *(ext++);
    *(j++) = *(ext++);
    *j = *ext;
    return true;
}

bool change_ext(char *str, size_t pos, size_t max_size, char const *ext)
{
    char *j = &str[pos];
    while (j > str && *j != '.')
        --j;
    if (j == str) return false;
    return append_ext(str, (size_t)(j - str), max_size, ext);
}

char *musl_basename(char *s)
{
    size_t i;
    if (!s || !*s) return ".";
    i = strlen(s) - 1;
    for (; i && s[i] == '/'; i--)
        s[i] = 0;
    for (; i && s[i - 1] != '/'; i--)
        ;
    return s + i;
}
