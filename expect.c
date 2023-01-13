#include "expect.h"
#include "array_size.h"
#include "rc.h"
#include <string.h>

int expect_map_size(struct lip_file *file, unsigned size)
{
  unsigned sz = 0;
  if (!lip_read_map_size(file, &sz)) return EFREAD;
  return size == sz ? 0 : EFDATA;
}

int expect_map_key(struct lip_file *file, char const key[])
{
  unsigned size = 0;
  char buf[16] = {0};

  if (!lip_read_str_size(file, &size)) return EFREAD;

  if (size > array_size(buf)) return EFDATA;

  if (!lip_read_str_data(file, size, buf)) return EFREAD;

  if (size != (unsigned)strlen(key)) return EFDATA;

  return strncmp(key, buf, size) ? EFDATA : 0;
}
