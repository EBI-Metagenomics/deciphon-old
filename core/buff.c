#include "deciphon/buff.h"
#include <assert.h>
#include <stdlib.h>

struct buff *buff_new(size_t size)
{
    assert(size > 0);
    return malloc(sizeof(struct buff) + size);
}

struct buff *buff_ensure(struct buff *buff, size_t size)
{
    if (size > buff->capacity)
    {
        struct buff *tmp = realloc(buff, sizeof(*tmp) + size);
        if (!tmp)
        {
            buff_del(buff);
            return 0;
        }
        buff = tmp;
        buff->capacity = size;
    }
    buff->size = size;
    return buff;
}

void buff_del(struct buff const *buff) { free((void *)buff); }
