#include "hmmer_result.h"
#include "deciphon/errno.h"
#include "h3c/h3c.h"
#include <stddef.h>

int hmmer_result_init(struct hmmer_result *x)
{
  return (x->handle = h3c_result_new()) ? 0 : DCP_ENOMEM;
}

void hmmer_result_cleanup(struct hmmer_result *x)
{
  if (x->handle) h3c_result_del(x->handle);
  x->handle = NULL;
}

int hmmer_result_nhits(struct hmmer_result const *x)
{
  return (int)h3c_result_nhits(x->handle);
}

int hmmer_result_pack(struct hmmer_result const *x, FILE *fp)
{
  return h3c_result_pack(x->handle, fp);
}
