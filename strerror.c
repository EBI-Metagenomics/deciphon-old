#include "deciphon/strerror.h"
#include "array_size.h"
#include "deciphon/errno.h"
#include <stdio.h>

static char const *msg[] = {
    [DCP_EDIFFABC] = "different alphabets",
    [DCP_EFCLOSE] = "failed to close file",
    [DCP_EFDATA] = "invalid file data",
    [DCP_EREFOPEN] = "failed to re-open file",
    [DCP_EFREAD] = "failed to read from file",
    [DCP_EFSEEK] = "failed to seek file",
    [DCP_EFTELL] = "failed to get file position",
    [DCP_EFUNCUSE] = "invalid function usage",
    [DCP_EFWRITE] = "failed to write to file",
    [DCP_EGETPATH] = "failed to get file path",
    [DCP_EZEROSEQ] = "zero-length sequence",
    [DCP_EZEROMODEL] = "zero-length model",
    [DCP_EZEROPART] = "no partition",
    [DCP_EDECODON] = "failed to decode into codon",
    [DCP_ELARGEMODEL] = "model is too large",
    [DCP_ELARGEPROTEIN] = "protein is too large",
    [DCP_EREADHMMER3] = "failed to read hmmer3 profile",
    [DCP_EMANYPARTS] = "too may partitions",
    [DCP_EMANYTRANS] = "too many transitions",
    [DCP_ENOMEM] = "not enough memory",
    [DCP_EOPENDB] = "failed to open DB file",
    [DCP_EOPENHMM] = "failed to open HMM file",
    [DCP_EOPENTMP] = "failed to open temporary file",
    [DCP_ETRUNCPATH] = "truncated file path",
    [DCP_EDPUNPACK] = "failed to unpack DP",
    [DCP_EDPPACK] = "failed to pack DP",
    [DCP_ENUCLTDUNPACK] = "failed to unpack nuclt dist",
    [DCP_ENUCLTDPACK] = "failed to pack nuclt dist",
    [DCP_ESETTRANS] = "failed to set transition",
    [DCP_EADDSTATE] = "failed to add state",
    [DCP_EDPRESET] = "failed to reset DP",
};

char const *dcp_strerror(int errno)
{
  if (errno > 0 && errno < (int)array_size(msg)) return msg[errno];

  static char unknown[32] = {0};
  snprintf(unknown, sizeof unknown, "unknown error #%d", errno);
  return unknown;
}
