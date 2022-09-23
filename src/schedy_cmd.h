#ifndef SCHEDY_CMD_H
#define SCHEDY_CMD_H

#include "core/cmd.h"

#define SCHEDY_CMD_MAP(X)                                                      \
    X(INVALID, invalid, "")                                                    \
    X(HELP, help, "")                                                          \
    X(CONNECT, connect, "URL API_KEY")                                         \
    X(ONLINE, online, "")                                                      \
    X(WIPE, wipe, "")                                                          \
    X(CANCEL, cancel, "")                                                      \
                                                                               \
    X(HMM_UP, hmm_up, "HMM_FILE")                                              \
    X(HMM_DL, hmm_dl, "XXH3 OUTPUT_FILE")                                      \
    X(HMM_GET_BY_ID, hmm_get_by_id, "HMM_ID")                                  \
    X(HMM_GET_BY_XXH3, hmm_get_by_xxh3, "XXH3")                                \
    X(HMM_GET_BY_JOB_ID, hmm_get_by_job_id, "JOB_ID")                          \
    X(HMM_GET_BY_FILENAME, hmm_get_by_filename, "FILENAME")                    \
                                                                               \
    X(DB_UP, db_up, "DB_FILE")                                                 \
    X(DB_DL, db_dl, "XXH3 OUTPUT_FILE")                                        \
    X(DB_GET_BY_ID, db_get_by_id, "DB_ID")                                     \
    X(DB_GET_BY_XXH3, db_get_by_xxh3, "XXH3")                                  \
    X(DB_GET_BY_HMM_ID, db_get_by_hmm_id, "HMM_ID")                            \
    X(DB_GET_BY_FILENAME, db_get_by_filename, "FILENAME")                      \
                                                                               \
    X(JOB_NEXT_PEND, job_next_pend, "")                                        \
    X(JOB_SET_STATE, job_set_state, "JOB_ID STATE [MSG]")                      \
    X(JOB_INC_PROGRESS, job_inc_progress, "JOB_ID PROGRESS")                   \
                                                                               \
    X(SCAN_DL_SEQS, scan_dl_seqs, "SCAN_ID FILE")                              \
    X(SCAN_GET_BY_JOB_ID, scan_get_by_job_id, "JOB_ID")                        \
    X(SCAN_SEQ_COUNT, scan_seq_count, "SCAN_ID")                               \
    X(SCAN_SUBMIT, scan_submit, "DB_ID MULTI_HITS HMMER3_COMPAT FASTA_FILE")   \
                                                                               \
    X(PRODS_FILE_UP, prods_file_up, "PRODS_FILE")

enum schedy_cmd
{
#define X(A, _1, _2) SCHEDY_CMD_##A,
    SCHEDY_CMD_MAP(X)
#undef X
};

cmd_fn_t *schedy_cmd(char const *cmd);

#define X(_1, A, _2) char const *schedy_cmd_##A(struct cmd const *);
SCHEDY_CMD_MAP(X)
#undef X

#endif
