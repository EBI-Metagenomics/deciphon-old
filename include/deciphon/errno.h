#ifndef DECIPHON_ERRNO_H
#define DECIPHON_ERRNO_H

#include "deciphon/export.h"

// clang-format off
enum dcp_errno
{
  DCP_EDIFFABC      = 1,
  DCP_EFCLOSE       = 2,
  DCP_EFDATA        = 3,
  DCP_EREFOPEN      = 4,
  DCP_EFREAD        = 5,
  DCP_EFSEEK        = 6,
  DCP_EFTELL        = 7,
  DCP_EFUNCUSE      = 8,
  DCP_EFWRITE       = 9,
  DCP_EGETPATH      = 10,
  DCP_EZEROSEQ      = 11,
  DCP_EZEROMODEL    = 12,
  DCP_EZEROPART     = 13,
  DCP_EDECODON      = 14,
  DCP_ELARGEMODEL   = 15,
  DCP_ELARGEPROTEIN = 16,
  DCP_EREADHMMER3   = 17,
  DCP_EMANYPARTS    = 18,
  DCP_EMANYTRANS    = 19,
  DCP_ENOMEM        = 20,
  DCP_EOPENDB       = 21,
  DCP_EOPENHMM      = 22,
  DCP_EOPENTMP      = 23,
  DCP_ETRUNCPATH    = 24,
  DCP_EDPUNPACK     = 25,
  DCP_EDPPACK       = 26,
  DCP_ENUCLTDUNPACK = 27,
  DCP_ENUCLTDPACK   = 28,
  DCP_ESETTRANS     = 29,
  DCP_EADDSTATE     = 30,
  DCP_EDPRESET      = 31,
};
// clang-format on

#endif