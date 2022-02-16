#include "xlip.h"
#include "imm/support.h"
#include "lite_pack/lite_pack.h"

bool xlip_expect_map(struct lip_io_file *io, unsigned size)
{
    unsigned sz = 0;
    lip_read_map_size(io, &sz);
    return size == sz;
}

bool xlip_expect_key(struct lip_io_file *io, char const key[])
{
    unsigned size = 0;
    char buf[16] = {0};

    lip_read_str_size(io, &size);
    if (size > IMM_ARRAY_SIZE(buf)) io->error = true;

    lip_read_str_data(io, size, buf);
    if (size != (unsigned)strlen(key)) io->error = true;
    return strncmp(key, buf, size) == 0;
}

bool xlip_expect_1darray_u8(struct lip_io_file *io, unsigned size,
                            uint8_t arr[])
{
    unsigned sz = 0;
    uint8_t type = 0;
    lip_read_1darray_size_type(io, &sz, &type);
    if (size != sz) goto error;
    lip_read_1darray_int_data(io, size, arr);
    return !io->error;

error:
    io->error = true;
    return false;
}

bool xlip_expect_1darray_u8_type(struct lip_io_file *io, unsigned size,
                                 uint8_t type, uint8_t arr[])
{
    unsigned sz = 0;
    uint8_t ty = 0;
    lip_read_1darray_size_type(io, &sz, &ty);
    if (size != sz) goto error;
    if (type != ty) goto error;
    lip_read_1darray_int_data(io, size, arr);
    return !io->error;

error:
    io->error = true;
    return false;
}

bool xlip_expect_1darray_float(struct lip_io_file *io, unsigned size,
                               imm_float arr[])
{
    unsigned sz = 0;
    uint8_t type = 0;
    lip_read_1darray_size_type(io, &sz, &type);
    if (size != sz) goto error;
    lip_read_1darray_float_data(io, size, arr);
    return !io->error;

error:
    io->error = true;
    return false;
}

bool xlip_expect_1darray_float_type(struct lip_io_file *io, unsigned size,
                                    uint8_t type, imm_float arr[])
{
    unsigned sz = 0;
    uint8_t ty = 0;
    lip_read_1darray_size_type(io, &sz, &type);
    if (size != sz) goto error;
    if (type != ty) goto error;
    lip_read_1darray_float_data(io, size, arr);
    return !io->error;

error:
    io->error = true;
    return false;
}

void xlip_write_cstr(struct lip_io_file *io, char const str[])
{
    unsigned size = (unsigned)strlen(str);
    lip_write_str_size(io, size);
    lip_write_str_data(io, size, str);
}

void xlip_read_cstr(struct lip_io_file *io, unsigned size, char str[])
{
    str[0] = 0;
    unsigned sz = 0;
    lip_read_str_size(io, &sz);
    if (sz > size)
    {
        io->error = true;
        return;
    }
    lip_read_str_data(io, sz, str);
    str[sz] = 0;
}
