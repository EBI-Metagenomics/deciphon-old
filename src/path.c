#include "path.h"
#include <string.h>

static bool append_ext(char *str, size_t len, size_t max_size, char const *ext)
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

static bool change_ext(char *str, size_t pos, size_t max_size, char const *ext)
{
    char *j = &str[pos];
    while (j > str && *j != '.')
        --j;
    if (j == str) return false;
    return append_ext(str, (size_t)(j - str), max_size, ext);
}

bool path_change_or_add_ext(char *str, size_t max_size, char const *ext)
{
    size_t len = strlen(str);
    if (!change_ext(str, len, max_size, ext))
        return append_ext(str, len, max_size, ext);
    return true;
}
