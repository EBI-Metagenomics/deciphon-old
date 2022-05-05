#ifndef DECIPHON_CORE_EXPECT_H
#define DECIPHON_CORE_EXPECT_H

#include "deciphon/core/compiler.h"
#include "deciphon/core/lite_pack.h"
#include "imm/float.h"
#include <stdbool.h>
#include <stdint.h>

#include <stdbool.h>
#include <stdint.h>

static inline bool expect_map_size(struct lip_file *file, unsigned size)
{
    unsigned sz = 0;
    lip_read_map_size(file, &sz);
    return size == sz;
}

static inline bool expect_map_key(struct lip_file *file, char const key[])
{
    unsigned size = 0;
    char buf[16] = {0};

    lip_read_str_size(file, &size);
    if (size > ARRAY_SIZE(buf)) file->error = true;

    lip_read_str_data(file, size, buf);
    if (size != (unsigned)strlen(key)) file->error = true;
    return strncmp(key, buf, size) == 0;
}

#endif
