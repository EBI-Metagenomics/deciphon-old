#include "expect.h"
#include "array_size.h"
#include "deciphon/errno.h"
#include <string.h>

int expect_map_size(struct lip_file *file, unsigned size)
{
  unsigned sz = 0;
  if (!lip_read_map_size(file, &sz)) return DCP_EFREAD;
  return size == sz ? 0 : DCP_EFDATA;
}

int expect_map_key(struct lip_file *file, char const key[])
{
  unsigned size = 0;
  char buf[32] = {0};

  if (!lip_read_str_size(file, &size)) return DCP_EFREAD;

  if (size > array_size(buf)) return DCP_EFDATA;

  if (!lip_read_str_data(file, size, buf)) return DCP_EFREAD;

  if (size != (unsigned)strlen(key)) return DCP_EFDATA;

  return strncmp(key, buf, size) ? DCP_EFDATA : 0;
}
