#include "basename.h"
#include <string.h>

#ifdef MS_WINDOWS
#define PATH_SEP '\\'
#endif

#ifndef PATH_SEP
#define PATH_SEP '/'
#endif

// Acknowledgment: gblic
char *basename(char const *path)
{
    char *p = strrchr(path, PATH_SEP);
    return p ? p + 1 : (char *)path;
}
