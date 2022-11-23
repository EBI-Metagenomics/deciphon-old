#include "dirname.h"
#include <stddef.h>
#include <string.h>

// Acknowledgment: musl
char *dirname_musl(char *path)
{
    size_t i = 0;
    if (!path || !*path) return ".";
    i = strlen(path) - 1;
    for (; path[i] == '/'; i--)
        if (!i) return "/";
    for (; path[i] != '/'; i--)
        if (!i) return ".";
    for (; path[i] == '/'; i--)
        if (!i) return "/";
    path[i + 1] = 0;
    return path;
}
