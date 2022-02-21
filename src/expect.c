#include "expect.h"
#include "dcp/compiler.h"
#include "lite_pack.h"

bool expect_map(struct lip_file *file, unsigned size)
{
    unsigned sz = 0;
    lip_read_map_size(file, &sz);
    return size == sz;
}

bool expect_key(struct lip_file *file, char const key[])
{
    unsigned size = 0;
    char buf[16] = {0};

    lip_read_str_size(file, &size);
    if (size > ARRAY_SIZE(buf)) file->error = true;

    lip_read_str_data(file, size, buf);
    if (size != (unsigned)strlen(key)) file->error = true;
    return strncmp(key, buf, size) == 0;
}

bool expect_1darray_u8(struct lip_file *file, unsigned size, uint8_t arr[])
{
    unsigned sz = 0;
    uint8_t type = 0;
    lip_read_1darray_size_type(file, &sz, &type);
    if (size != sz) goto error;
    lip_read_1darray_int_data(file, size, arr);
    return !file->error;

error:
    file->error = true;
    return false;
}

bool expect_1darray_u8_type(struct lip_file *file, unsigned size, uint8_t type,
                            uint8_t arr[])
{
    unsigned sz = 0;
    uint8_t ty = 0;
    lip_read_1darray_size_type(file, &sz, &ty);
    if (size != sz) goto error;
    if (type != ty) goto error;
    lip_read_1darray_int_data(file, size, arr);
    return !file->error;

error:
    file->error = true;
    return false;
}

bool expect_1darray_float(struct lip_file *file, unsigned size, imm_float arr[])
{
    unsigned sz = 0;
    uint8_t type = 0;
    lip_read_1darray_size_type(file, &sz, &type);
    if (size != sz) goto error;
    lip_read_1darray_float_data(file, size, arr);
    return !file->error;

error:
    file->error = true;
    return false;
}

bool expect_1darray_float_type(struct lip_file *file, unsigned size,
                               uint8_t type, imm_float arr[])
{
    unsigned sz = 0;
    uint8_t ty = 0;
    lip_read_1darray_size_type(file, &sz, &type);
    if (size != sz) goto error;
    if (type != ty) goto error;
    lip_read_1darray_float_data(file, size, arr);
    return !file->error;

error:
    file->error = true;
    return false;
}
