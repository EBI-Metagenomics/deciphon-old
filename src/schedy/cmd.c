#include "schedy/cmd.h"
#include "core/api.h"
#include "core/file.h"
#include "core/logy.h"
#include "core/sched_dump.h"
#include "schedy/strings.h"
#include "xfile.h"
#include <string.h>

#define CMD_MAP(X)                                                             \
    X(INVALID, invalid, "")                                                    \
    X(HELP, help, "")                                                          \
    X(SETUP, setup, "URL API_KEY")                                             \
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

#define CMD_TEMPLATE_ENABLE
#include "core/cmd_template.h"
#undef CMD_TEMPLATE_ENABLE

static enum rc download_hmm(char const *filepath, void *data);
static enum rc download_db(char const *filepath, void *data);
static bool encode_job_state(char const *str, enum sched_job_state *);

static char buffer[6 * 1024 * 1024] = {0};

static char const *fn_invalid(struct cmd *cmd)
{
    if (!cmd_check(cmd, "s")) return eparse(FAIL_PARSE), FAIL;
    return eparse("invalid command"), FAIL;
}

static char const *fn_help(struct cmd *cmd)
{
    if (!cmd_check(cmd, "s")) return eparse(FAIL_PARSE), FAIL;

    static char help_table[1024] = {0};
    char *p = help_table;
    p += sprintf(p, "Commands:");

#define X(_, A, B)                                                             \
    if (strcmp(STRINGIFY(A), "invalid"))                                       \
        p += sprintf(p, "\n  %-22s %s", STRINGIFY(A), B);
    CMD_MAP(X);
#undef X

    return help_table;
}

static char const *fn_setup(struct cmd *cmd)
{
    if (!cmd_check(cmd, "sss")) return eparse(FAIL_PARSE), FAIL;
    if (api_init(cmd_get(cmd, 1), cmd_get(cmd, 2))) return FAIL;
    return OK;
}

static char const *fn_online(struct cmd *cmd)
{
    if (!cmd_check(cmd, "s")) return eparse(FAIL_PARSE), FAIL;
    return api_is_reachable() ? YES : NO;
}

static char const *fn_wipe(struct cmd *cmd)
{
    if (!cmd_check(cmd, "s")) return eparse(FAIL_PARSE), FAIL;
    return api_wipe() ? FAIL : OK;
}

static char const *fn_cancel(struct cmd *cmd)
{
    if (!cmd_check(cmd, "s")) return eparse(FAIL_PARSE), FAIL;
    return OK;
}

static char const *fn_hmm_up(struct cmd *cmd)
{
    if (!cmd_check(cmd, "ss")) return eparse(FAIL_PARSE), FAIL;
    static struct sched_hmm hmm = {0};
    return api_hmm_up(cmd_get(cmd, 1), &hmm) ? FAIL : OK;
}

static char const *fn_hmm_dl(struct cmd *cmd)
{
    if (!cmd_check(cmd, "sis")) return eparse(FAIL_PARSE), FAIL;

    int64_t xxh3 = cmd_as_i64(cmd, 1);
    if (file_ensure_local(cmd_get(cmd, 2), xxh3, &download_hmm, &xxh3))
        return FAIL;
    else
        return OK;
}

static char const *fn_hmm_get_by_id(struct cmd *cmd)
{
    if (!cmd_check(cmd, "si")) return eparse(FAIL_PARSE), FAIL;
    static struct sched_hmm hmm = {0};
    if (api_hmm_get_by_id(cmd_as_i64(cmd, 1), &hmm)) return FAIL;
    return sched_dump_hmm(&hmm, (char *)buffer);
}

static char const *fn_hmm_get_by_xxh3(struct cmd *cmd)
{
    if (!cmd_check(cmd, "si")) return eparse(FAIL_PARSE), FAIL;
    static struct sched_hmm hmm = {0};
    if (api_hmm_get_by_xxh3(cmd_as_i64(cmd, 1), &hmm)) return FAIL;
    return sched_dump_hmm(&hmm, (char *)buffer);
}

static char const *fn_hmm_get_by_job_id(struct cmd *cmd)
{
    if (!cmd_check(cmd, "si")) return eparse(FAIL_PARSE), FAIL;
    static struct sched_hmm hmm = {0};
    if (api_hmm_get_by_job_id(cmd_as_i64(cmd, 1), &hmm)) return FAIL;
    return sched_dump_hmm(&hmm, (char *)buffer);
}

static char const *fn_hmm_get_by_filename(struct cmd *cmd)
{
    if (!cmd_check(cmd, "ss")) return eparse(FAIL_PARSE), FAIL;
    static struct sched_hmm hmm = {0};
    if (api_hmm_get_by_filename(cmd_get(cmd, 1), &hmm)) return FAIL;
    return sched_dump_hmm(&hmm, (char *)buffer);
}

static char const *fn_db_up(struct cmd *cmd)
{
    if (!cmd_check(cmd, "ss")) return eparse(FAIL_PARSE), FAIL;
    static struct sched_db db = {0};
    return api_db_up(cmd_get(cmd, 1), &db) ? FAIL : OK;
}

static char const *fn_db_dl(struct cmd *cmd)
{
    if (!cmd_check(cmd, "sis")) return eparse(FAIL_PARSE), FAIL;
    int64_t xxh3 = cmd_as_i64(cmd, 1);
    if (file_ensure_local(cmd_get(cmd, 2), xxh3, &download_db, &xxh3))
        return FAIL;
    else
        return OK;
}

static char const *fn_db_get_by_id(struct cmd *cmd)
{
    if (!cmd_check(cmd, "si")) return eparse(FAIL_PARSE), FAIL;
    static struct sched_db db = {0};
    if (api_db_get_by_id(cmd_as_i64(cmd, 1), &db)) return FAIL;
    return sched_dump_db(&db, (char *)buffer);
}

static char const *fn_db_get_by_xxh3(struct cmd *cmd)
{
    if (!cmd_check(cmd, "si")) return eparse(FAIL_PARSE), FAIL;
    static struct sched_db db = {0};
    if (api_db_get_by_xxh3(cmd_as_i64(cmd, 1), &db)) return FAIL;
    return sched_dump_db(&db, (char *)buffer);
}

static char const *fn_db_get_by_hmm_id(struct cmd *cmd)
{
    if (!cmd_check(cmd, "si")) return eparse(FAIL_PARSE), FAIL;
    static struct sched_db db = {0};
    if (api_db_get_by_hmm_id(cmd_as_i64(cmd, 1), &db)) return FAIL;
    return sched_dump_db(&db, (char *)buffer);
}

static char const *fn_db_get_by_filename(struct cmd *cmd)
{
    if (!cmd_check(cmd, "ss")) return eparse(FAIL_PARSE), FAIL;
    static struct sched_db db = {0};
    if (api_db_get_by_filename(cmd_get(cmd, 1), &db)) return FAIL;
    return sched_dump_db(&db, (char *)buffer);
}

static char const *fn_job_next_pend(struct cmd *cmd)
{
    if (!cmd_check(cmd, "s")) return eparse(FAIL_PARSE), FAIL;
    static struct sched_job job = {0};
    if (api_job_next_pend(&job)) return FAIL;
    return sched_dump_job(&job, (char *)buffer);
}

static char const *fn_job_set_state(struct cmd *cmd)
{
    static char const empty[] = "";
    char const *msg = empty;
    if (cmd->argc == 3)
    {
        if (!cmd_check(cmd, "sis")) return eparse(FAIL_PARSE), FAIL;
    }
    else
    {
        if (!cmd_check(cmd, "siss")) return eparse(FAIL_PARSE), FAIL;
        msg = cmd_get(cmd, 3);
    }
    enum sched_job_state state = 0;
    if (!encode_job_state(cmd_get(cmd, 2), &state)) return FAIL;
    int64_t job_id = cmd_as_i64(cmd, 1);
    return api_job_set_state(job_id, state, msg) ? FAIL : OK;
}

static char const *fn_job_inc_progress(struct cmd *cmd)
{
    if (!cmd_check(cmd, "sii")) return eparse(FAIL_PARSE), FAIL;
    int64_t job_id = cmd_as_i64(cmd, 1);
    int64_t increment = cmd_as_i64(cmd, 2);
    return api_job_inc_progress(job_id, increment) ? FAIL : OK;
}

static char const *fn_scan_dl_seqs(struct cmd *cmd)
{
    static struct sched_scan scan = {0};
    static char filepath[PATH_SIZE] = {0};

    if (!cmd_check(cmd, "sis")) return eparse(FAIL_PARSE), FAIL;

    if (api_scan_get_by_id(cmd_as_i64(cmd, 1), &scan)) return FAIL;

    int rc = xfile_mkstemp(sizeof filepath, filepath);
    if (rc)
    {
        eio("%s", xfile_strerror(rc));
        return FAIL;
    }
    FILE *fp = fopen(filepath, "wb");
    if (!fp)
    {
        eio("fopen failed");
        xfile_unlink(filepath);
        return FAIL;
    }

    if (api_scan_dl_seqs(scan.id, fp))
    {
        fclose(fp);
        return FAIL;
    }

    fclose(fp);
    rc = xfile_move(cmd_get(cmd, 2), filepath);
    if (rc)
    {
        eio("%s", xfile_strerror(rc));
        return FAIL;
    }
    return cmd_get(cmd, 2);
}

static char const *fn_scan_get_by_job_id(struct cmd *cmd)
{
    if (!cmd_check(cmd, "si")) return eparse(FAIL_PARSE), FAIL;
    static struct sched_scan scan = {0};
    if (api_scan_get_by_job_id(cmd_as_i64(cmd, 1), &scan)) return FAIL;
    return sched_dump_scan(&scan, (char *)buffer);
}

static char const *fn_scan_seq_count(struct cmd *cmd)
{
    if (!cmd_check(cmd, "si")) return eparse(FAIL_PARSE), FAIL;
    unsigned count = 0;
    if (api_scan_seq_count(cmd_as_i64(cmd, 1), &count)) return FAIL;
    sprintf((char *)buffer, "%u", count);
    return buffer;
}

static char const *fn_scan_submit(struct cmd *cmd)
{
    if (!cmd_check(cmd, "siiis")) return eparse(FAIL_PARSE), FAIL;
    int64_t db_id = cmd_as_i64(cmd, 1);
    int64_t multi_hits = cmd_as_i64(cmd, 2);
    int64_t hmmer3_compat = cmd_as_i64(cmd, 3);
    static struct sched_job job = {0};
    if (api_scan_submit(db_id, multi_hits, hmmer3_compat, cmd_get(cmd, 4),
                        &job))
        return FAIL;
    return sched_dump_job(&job, (char *)buffer);
}

static char const *fn_prods_file_up(struct cmd *cmd)
{
    (void)cmd;
    return "";
}

static enum rc download_hmm(char const *filepath, void *data)
{
    static struct sched_hmm hmm = {0};

    int64_t xxh3 = *((int64_t *)data);
    enum rc rc = api_hmm_get_by_xxh3(xxh3, &hmm);
    if (rc) return rc;

    FILE *fp = fopen(filepath, "wb");
    if (!fp) return eio("fopen");
    if ((rc = api_hmm_dl(hmm.id, fp)))
    {
        fclose(fp);
        return rc;
    }
    return fclose(fp) ? eio("fclose") : RC_OK;
}

static enum rc download_db(char const *filepath, void *data)
{
    static struct sched_db db = {0};

    int64_t xxh3 = *((int64_t *)data);
    enum rc rc = api_db_get_by_xxh3(xxh3, &db);
    if (rc) return rc;

    FILE *fp = fopen(filepath, "wb");
    if (!fp) return eio("fopen");
    if ((rc = api_db_dl(db.id, fp)))
    {
        fclose(fp);
        return rc;
    }
    return fclose(fp) ? eio("fclose") : RC_OK;
}

static bool encode_job_state(char const *str, enum sched_job_state *state)
{
    static char const strings[][5] = {"pend", "run", "done", "fail"};
    for (int i = 0; i < 4; ++i)
    {
        if (!strcmp(strings[i], str))
        {
            *state = i;
            return true;
        }
    }
    einval("invalid job state string");
    return false;
}
