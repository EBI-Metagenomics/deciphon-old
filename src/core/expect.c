#include "core/expect.h"
#include "core/pp.h"
#include <string.h>

bool expect_map_size(struct lip_file *file, unsigned size)
{
    unsigned sz = 0;
    lip_read_map_size(file, &sz);
    return size == sz;
}

bool expect_map_key(struct lip_file *file, char const key[])
{
    unsigned size = 0;
    char buf[16] = {0};

    lip_read_str_size(file, &size);
    if (size > ARRAY_SIZE(buf)) file->error = true;

    lip_read_str_data(file, size, buf);
    if (size != (unsigned)strlen(key)) file->error = true;
    return strncmp(key, buf, size) == 0;
}
