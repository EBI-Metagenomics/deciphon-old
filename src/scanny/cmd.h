#ifndef SCANNY_CMD_H
#define SCANNY_CMD_H

#include "core/cmd.h"

#define CMD_MAP(X)                                                             \
    X(INVALID, invalid, "")                                                    \
    X(HELP, help, "")                                                          \
    X(SET_NTHREADS, set_nthreads, "NTHREADS")                                  \
    X(SCAN, scan, "SEQS_FILE DB_FILE PROD_FILE MULTI_HITS HMMER3_COMPAT")      \
    X(CANCEL, cancel, "")                                                      \
    X(STATE, state, "")                                                        \
    X(PROGRESS, progress, "")

enum
{
#define X(A, _1, _2) CMD_##A,
    CMD_MAP(X)
#undef X
};

cmd_fn_t *cmd_get_callback(char const *cmd);

#define X(_1, A, _2) char const *cmd_##A(struct cmd const *);
CMD_MAP(X)
#undef X

#endif
