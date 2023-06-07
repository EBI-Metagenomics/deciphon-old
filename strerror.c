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
    [DCP_EFSTAT] = "failed to get file stat",
    [DCP_EFOPEN] = "failed to open file",
    [DCP_ELARGEFILE] = "file is too large",
    [DCP_ELONGPATH] = "path is too long",
    [DCP_EIMMRESETTASK] = "failed to reset task",
    [DCP_EIMMNEWTASK] = "failed to create new task",
    [DCP_EIMMSETUPTASK] = "failed to setup task",
    [DCP_EWRITEPROD] = "failed to write product",
    [DCP_EINVALPART] = "invalid partition",
    [DCP_ELONGACC] = "accession string is too long",
    [DCP_EMANYTHREADS] = "too many threads",
    [DCP_ETMPFILE] = "failed to create temporary file",
    [DCP_EFFLUSH] = "failed to flush file",
    [DCP_EMKDIR] = "failed to create directory",
    [DCP_EH3CDIAL] = "failed to dial to h3daemon",
    [DCP_EH3CPUT] = "failed to send to h3daemon",
    [DCP_EH3CPOP] = "failed to get from h3daemon",
    [DCP_EH3CPACK] = "failed to package h3daemon file",
    [DCP_EFORMAT] = "wrong file format",
    [DCP_ERMDIR] = "failed to remove directory",
    [DCP_ERMFILE] = "failed to remove file",
    [DCP_ESETGENCODE] = "must set gencode first",
    [DCP_EGENCODEID] = "invalid gencode id",
    [DCP_EELAPSED] = "elapsed library failed",
    [DCP_EMAXRETRY] = "reached maximum number of retries",
};

char const *dcp_strerror(int errno)
{
  if (errno > 0 && errno < (int)array_size(msg)) return msg[errno];

  static char unknown[32] = {0};
  snprintf(unknown, sizeof unknown, "unknown error #%d", errno);
  return unknown;
}
