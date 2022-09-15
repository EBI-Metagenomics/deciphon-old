#ifndef SCHEDY_CMD_H
#define SCHEDY_CMD_H

#include "core/cmd.h"

#define SCHEDY_CMD_MAP(X)                                                      \
    X(INVALID, schedy_cmd_invalid)                                             \
    X(CONNECT, schedy_cmd_connect)                                             \
    X(ONLINE, schedy_cmd_online)                                               \
    X(WIPE, schedy_cmd_wipe)                                                   \
                                                                               \
    X(HMM_UP, schedy_cmd_hmm_up)                                               \
    X(HMM_DL, schedy_cmd_hmm_dl)                                               \
    X(HMM_GET_BY_ID, schedy_cmd_hmm_get_by_id)                                 \
    X(HMM_GET_BY_XXH3, schedy_cmd_hmm_get_by_xxh3)                             \
    X(HMM_GET_BY_JOB_ID, schedy_cmd_hmm_get_by_job_id)                         \
    X(HMM_GET_BY_FILENAME, schedy_cmd_hmm_get_by_filename)                     \
                                                                               \
    X(DB_UP, schedy_cmd_db_up)                                                 \
    X(DB_DL, schedy_cmd_db_dl)                                                 \
    X(DB_GET_BY_ID, schedy_cmd_db_get_by_id)                                   \
    X(DB_GET_BY_XXH3, schedy_cmd_db_get_by_xxh3)                               \
    X(DB_GET_BY_HMM_ID, schedy_cmd_db_get_by_hmm_id)                           \
    X(DB_GET_BY_FILENAME, schedy_cmd_db_get_by_filename)                       \
                                                                               \
    X(JOB_NEXT_PEND, schedy_cmd_job_next_pend)                                 \
    X(JOB_SET_STATE, schedy_cmd_job_set_state)                                 \
    X(JOB_INC_PROGRESS, schedy_cmd_job_inc_progress)                           \
                                                                               \
    X(SCAN_DL_SEQS, schedy_cmd_scan_dl_seqs)                                   \
    X(SCAN_GET_BY_JOB_ID, schedy_cmd_scan_get_by_job_id)                       \
    X(SCAN_NEXT_SEQ, schedy_cmd_scan_next_seq)                                 \
    X(SCAN_SEQ_COUNT, schedy_cmd_scan_seq_count)                               \
    X(SCAN_SUBMIT, schedy_cmd_scan_submit)                                     \
                                                                               \
    X(PRODS_FILE_UP, schedy_cmd_prods_file_up)

enum schedy_cmd
{
#define X(A, _) SCHEDY_CMD_##A,
    SCHEDY_CMD_MAP(X)
#undef X
};

cmd_fn_t *schedy_cmd(char const *cmd);

#define X(_, X) char const *X(struct cmd const *);
SCHEDY_CMD_MAP(X)
#undef X

#endif
