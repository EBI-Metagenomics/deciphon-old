#include "strkcpy.h"
#include "strlcpy.h"

bool strkcpy(char *dst, char const *src, size_t dsize)
{
  return imm_strlcpy(dst, src, dsize) < dsize;
}
