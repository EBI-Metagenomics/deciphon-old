#ifndef SCANNY_CMD_H
#define SCANNY_CMD_H

#include "core/cmd.h"

#define SCANNY_CMD_MAP(X)                                                      \
    X(INVALID, invalid, "")                                                    \
    X(HELP, help, "")                                                          \
    X(SET_NTHREADS, set_nthreads, "NTHREADS")                                  \
    X(SCAN, scan, "SEQS_FILE DB_FILE PROD_FILE MULTI_HITS HMMER3_COMPAT")      \
    X(CANCEL, cancel, "")                                                      \
    X(STATE, state, "")                                                        \
    X(PROGRESS, progress, "")

enum scanny_cmd
{
#define X(A, _1, _2) SCANNY_CMD_##A,
    SCANNY_CMD_MAP(X)
#undef X
};

cmd_fn_t *scanny_cmd(char const *cmd);

#define X(_1, A, _2) char const *scanny_cmd_##A(struct cmd const *);
SCANNY_CMD_MAP(X)
#undef X

#endif
