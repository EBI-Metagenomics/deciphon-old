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
  DCP_EFSTAT        = 32,
  DCP_EFOPEN        = 33,
  DCP_ELARGEFILE    = 34,
  DCP_EJSONINVAL    = 35,
  DCP_EJSONRANGE    = 36,
  DCP_EJSONFOUND    = 37,
  DCP_ELARGEPATH    = 38,
  DCP_EIMMRESETTASK = 39,
  DCP_EIMMNEWTASK   = 40,
  DCP_EIMMSETUPTASK = 41,
  DCP_EWRITEPROD    = 42,
  DCP_EINVALPART    = 43,
  DCP_ELARGEACC     = 44,
  DCP_EMANYTHREADS  = 45,
  DCP_ETMPFILE      = 46,
  DCP_EFFLUSH       = 47,
};
// clang-format on

#endif
