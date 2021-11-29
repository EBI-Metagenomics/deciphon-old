#include "path.h"
#include "xstrlcpy.h"
#include <string.h>

bool append_ext(char *str, size_t len, size_t max_size, char const *ext);
bool change_ext(char *str, size_t pos, size_t max_size, char const *ext);
char *glibc_basename(const char *filename);

bool path_change_or_add_ext(char str[static 1], size_t max_size,
                            char const ext[static 1])
{
    size_t len = strlen(str);
    if (!change_ext(str, len, max_size, ext))
        return append_ext(str, len, max_size, ext);
    return true;
}

void path_basename(char *filename, char const *path)
{
    char *p = glibc_basename(path);
    xstrlcpy(filename, p, DCP_FILENAME_SIZE);
}

void path_strip_ext(char str[static 1])
{
    char *ret = strrchr(str, '.');
    if (ret) *ret = 0;
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

char *glibc_basename(const char *filename)
{
    char *p = strrchr(filename, '/');
    return p ? p + 1 : (char *)filename;
}
