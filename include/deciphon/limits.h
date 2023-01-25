#ifndef DECIPHON_LIMITS_H
#define DECIPHON_LIMITS_H

#include <assert.h>

enum dcp_limits
{
  DCP_ABC_NAME_SIZE = 16,
  DCP_PROFILE_NAME_SIZE = 64,
  DCP_VERSION_SIZE = 16,
  DCP_NPARTITIONS_SIZE = 128,
  DCP_NTHREADS_SIZE = 128,
};

static_assert(DCP_NPARTITIONS_SIZE == DCP_NTHREADS_SIZE, "");

#endif
