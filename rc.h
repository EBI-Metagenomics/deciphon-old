#ifndef RC_H
#define RC_H

#define RC_MAP(X)                                                              \
  X(OK, "ok")                                                                  \
  X(EDIFFABC, "different alphabets")                                           \
  X(EFCLOSE, "failed to close file")                                           \
  X(EFDATA, "invalid file data")                                               \
  X(EFOPEN, "failed to open file")                                             \
  X(EFREAD, "failed to read from file")                                        \
  X(EFSEEK, "failed to seek file")                                             \
  X(EFTELL, "failed to get file position")                                     \
  X(EFUNCUSE, "invalid function usage")                                        \
  X(EFWRITE, "failed to write to file")                                        \
  X(EGETPATH, "failed to get file path")                                       \
  X(EINVAL, "invalid value")                                                   \
  X(ELARGEMODEL, "model is too large")                                         \
  X(ELARGEPROFILE, "profile is too large")                                     \
  X(EREADHMMER3, "failed to read hmmer3 profile")                              \
  X(EMANYPARTS, "too may partitions")                                          \
  X(EMANYTRANS, "too many transitions")                                        \
  X(ENOMEM, "not enough memory")                                               \
  X(EOPENDB, "failed to open DB file")                                         \
  X(EOPENHMM, "failed to open HMM file")                                       \
  X(EOPENTMP, "failed to open temporary file")                                 \
  X(ETRUNCPATH, "truncated file path")                                         \
  X(EDPUNPACK, "failed to unpack DP")                                          \
  X(EDPPACK, "failed to pack DP")                                              \
  X(ENUCLTDUNPACK, "failed to unpack nuclt dist")                              \
  X(ENUCLTDPACK, "failed to pack nuclt dist")                                  \
  X(ESETTRANS, "failed to set transition")                                     \
  X(EADDSTATE, "failed to add state")                                          \
  X(EDPRESET, "failed to reset DP")                                            \
  X(EEOF, "unexpected end of file")

enum
{
#define X(A, _) A,
  RC_MAP(X)
#undef X
      RC_SIZE,
};

char const *rc_string(int rc);

#endif
