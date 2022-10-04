#include "core/str.h"
#include <ctype.h>

bool str_all_spaces(char const *string)
{
    char const *p = string;
    unsigned count = 0;
    while (*p)
    {
        count += !isspace(*p);
        ++p;
    }
    return count == 0;
}
