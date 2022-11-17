#include "command.h"
#include "api.h"
#include "core/as.h"
#include "core/cmd.h"
#include "core/file.h"
#include "core/logy.h"
#include "core/msg.h"
#include "core/sched_dump.h"
#include "core/strings.h"
#include "fs.h"
#include "schedy.h"
#include <string.h>

#define CMD_MAP(X)                                                             \
    X(HELP, help, "")                                                          \
    X(ECHO, echo, "[...]")                                                     \
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
    X(JOB_GET_BY_ID, job_get_by_id, "JOB_ID")                                  \
                                                                               \
    X(SCAN_DL_SEQS, scan_dl_seqs, "SCAN_ID FILE")                              \
    X(SCAN_GET_BY_ID, scan_get_by_id, "SCAN_ID")                               \
    X(SCAN_GET_BY_JOB_ID, scan_get_by_job_id, "JOB_ID")                        \
    X(SCAN_SEQ_COUNT, scan_seq_count, "SCAN_ID")                               \
    X(SCAN_SUBMIT, scan_submit, "DB_ID MULTI_HITS HMMER3_COMPAT FASTA_FILE")   \
                                                                               \
    X(PRODS_FILE_UP, prods_file_up, "PRODS_FILE")                              \
    X(PRODS_FILE_DL, prods_file_dl, "SCAN_ID FILE")

#define COMMAND_TEMPLATE_DEF
#include "core/command_template.h"
#undef COMMAND_TEMPLATE_DEF

static enum rc dl_hmm(char const *filepath, void *data);
static enum rc dl_db(char const *filepath, void *data);
static bool encode_job_state(char const *str, enum sched_job_state *);

static char buffer[6 * 1024 * 1024] = {0};
static struct sched_hmm hmm = {0};
static struct sched_db db = {0};
static struct sched_job job = {0};
enum sched_job_state state = 0;
static struct sched_scan scan = {0};

static void fn_echo(struct msg *msg) { parent_send(&parent, msg_unparse(msg)); }

static void fn_help(struct msg *msg)
{
    unused(msg);
    cmd_help_init();

#define X(_, A, B) cmd_help_add(stringify(A), B);
    CMD_MAP(X);
#undef X

    parent_send(&parent, cmd_help_table());
}

static void fn_online(struct msg *msg)
{
    if (msg_check(msg, "s")) return;

    char const *ans = api_is_reachable() ? YES : NO;
    parent_send(&parent, msg_ctx(msg, ans));
}

static void fn_wipe(struct msg *msg)
{
    if (msg_check(msg, "s")) return;

    char const *ans = api_wipe() ? FAIL : OK;
    parent_send(&parent, msg_ctx(msg, ans));
}

static void fn_cancel(struct msg *msg)
{
    unused(msg);
    debug("not implemented yet");
}

static void fn_hmm_up(struct msg *msg)
{
    if (msg_check(msg, "ss")) return;

    char const *ans = api_hmm_up(msg_str(msg, 1), &hmm) ? FAIL : OK;
    parent_send(&parent, msg_ctx(msg, ans));
}

static void fn_hmm_dl(struct msg *msg)
{
    if (msg_check(msg, "sis")) return;

    long xxh3 = msg_int(msg, 1);
    char const *name = msg_str(msg, 2);
    char const *ans = file_ensure_local(name, xxh3, &dl_hmm, &xxh3) ? FAIL : OK;
    parent_send(&parent, msg_ctx(msg, ans));
}

static void fn_hmm_get_by_id(struct msg *msg)
{
    if (msg_check(msg, "si")) return;

    char const *ans = FAIL;
    char const *json = "";
    if (!api_hmm_get_by_id(msg_int(msg, 1), &hmm))
    {
        ans = OK;
        json = sched_dump_hmm(&hmm, (char *)buffer);
    }
    parent_send(&parent, msg_ctx(msg, ans, json));
}

static void fn_hmm_get_by_xxh3(struct msg *msg)
{
    if (msg_check(msg, "si")) return;

    char const *ans = FAIL;
    char const *json = "";
    if (!api_hmm_get_by_xxh3(msg_int(msg, 1), &hmm))
    {
        ans = OK;
        json = sched_dump_hmm(&hmm, (char *)buffer);
    }
    parent_send(&parent, msg_ctx(msg, ans, json));
}

static void fn_hmm_get_by_job_id(struct msg *msg)
{
    if (msg_check(msg, "si")) return;

    char const *ans = FAIL;
    char const *json = "";
    if (!api_hmm_get_by_job_id(msg_int(msg, 1), &hmm))
    {
        ans = OK;
        json = sched_dump_hmm(&hmm, (char *)buffer);
    }
    parent_send(&parent, msg_ctx(msg, ans, json));
}

static void fn_hmm_get_by_filename(struct msg *msg)
{
    if (msg_check(msg, "ss")) return;

    char const *ans = FAIL;
    char const *json = "";
    if (!api_hmm_get_by_filename(msg_str(msg, 1), &hmm))
    {
        ans = OK;
        json = sched_dump_hmm(&hmm, (char *)buffer);
    }
    parent_send(&parent, msg_ctx(msg, ans, json));
}

static void fn_db_up(struct msg *msg)
{
    if (msg_check(msg, "ss")) return;

    char const *ans = api_db_up(msg_str(msg, 1), &db) ? FAIL : OK;
    parent_send(&parent, msg_ctx(msg, ans));
}

static void fn_db_dl(struct msg *msg)
{
    if (msg_check(msg, "sis")) return;

    char const *ans = FAIL;
    long xxh3 = msg_int(msg, 1);
    char const *name = msg_str(msg, 2);
    if (!file_ensure_local(name, xxh3, &dl_db, &xxh3)) ans = OK;

    parent_send(&parent, msg_ctx(msg, ans));
}

static void fn_db_get_by_id(struct msg *msg)
{
    if (msg_check(msg, "si")) return;

    char const *ans = FAIL;
    char const *json = "";
    if (!api_db_get_by_id(msg_int(msg, 1), &db))
    {
        ans = OK;
        json = sched_dump_db(&db, (char *)buffer);
    }
    parent_send(&parent, msg_ctx(msg, ans, json));
}

static void fn_db_get_by_xxh3(struct msg *msg)
{
    if (msg_check(msg, "si")) return;

    char const *ans = FAIL;
    char const *json = "";
    if (!api_db_get_by_xxh3(msg_int(msg, 1), &db))
    {
        ans = OK;
        json = sched_dump_db(&db, (char *)buffer);
    }
    parent_send(&parent, msg_ctx(msg, ans, json));
}

static void fn_db_get_by_hmm_id(struct msg *msg)
{
    if (msg_check(msg, "si")) return;

    char const *ans = FAIL;
    char const *json = "";
    if (!api_db_get_by_hmm_id(msg_int(msg, 1), &db))
    {
        ans = OK;
        json = sched_dump_db(&db, (char *)buffer);
    }
    parent_send(&parent, msg_ctx(msg, ans, json));
}

static void fn_db_get_by_filename(struct msg *msg)
{
    if (msg_check(msg, "ss")) return;

    char const *ans = FAIL;
    char const *json = "";
    if (!api_db_get_by_filename(msg_str(msg, 1), &db))
    {
        ans = OK;
        json = sched_dump_db(&db, (char *)buffer);
    }
    parent_send(&parent, msg_ctx(msg, ans, json));
}

static void fn_job_next_pend(struct msg *msg)
{
    if (msg_check(msg, "s")) return;

    char const *ans = FAIL;
    char const *json = "";
    enum rc rc = api_job_next_pend(&job);
    if (!rc || rc == RC_END)
    {
        ans = rc == RC_END ? END : OK;
        json = rc != RC_END ? sched_dump_job(&job, (char *)buffer) : "";
    }
    parent_send(&parent, msg_ctx(msg, ans, json));
}

static void fn_job_set_state(struct msg *msg)
{
    if (msg_argc(msg) == 3 && msg_check(msg, "sis")) return;
    if (msg_check(msg, "sis*")) return;

    char const *ans = FAIL;
    char const *json = "";

    if (!encode_job_state(msg_str(msg, 2), &state)) goto cleanup;

    long job_id = msg_int(msg, 1);
    char const *error = msg_argc(msg) == 3 ? "" : msg_str(msg, 3);
    if (api_job_set_state(job_id, state, error)) goto cleanup;
    if (api_job_get_by_id(job_id, &job)) goto cleanup;
    ans = OK;
    json = sched_dump_job(&job, (char *)buffer);

cleanup:
    parent_send(&parent, msg_ctx(msg, ans, json));
}

static void fn_job_inc_progress(struct msg *msg)
{
    if (msg_check(msg, "sii")) return;

    long job_id = msg_int(msg, 1);
    long increment = msg_int(msg, 2);
    char const *ans = api_job_inc_progress(job_id, increment) ? FAIL : OK;

    parent_send(&parent, msg_ctx(msg, ans));
}

static void fn_job_get_by_id(struct msg *msg)
{
    if (msg_check(msg, "si")) return;

    char const *ans = FAIL;
    char const *json = "";
    if (!api_job_get_by_id(msg_int(msg, 1), &job))
    {
        ans = OK;
        json = sched_dump_job(&job, (char *)buffer);
    }
    parent_send(&parent, msg_ctx(msg, ans, json));
}

static void fn_scan_dl_seqs(struct msg *msg)
{
    if (msg_check(msg, "sis")) return;

    char const *ans = FAIL;
    char const *filepath = msg_str(msg, 2);
    static char tmpfpath[FILENAME_SIZE] = {0};

    if (api_scan_get_by_id(msg_int(msg, 1), &scan)) goto cleanup;

    int rc = fs_mkstemp(sizeof tmpfpath, tmpfpath);
    if (rc)
    {
        eio("%s", fs_strerror(rc));
        goto cleanup;
    }
    FILE *fp = fopen(tmpfpath, "wb");
    if (!fp)
    {
        eio("fopen failed");
        fs_unlink(tmpfpath);
        goto cleanup;
    }

    if (api_scan_dl_seqs(scan.id, fp))
    {
        fclose(fp);
        goto cleanup;
    }

    fclose(fp);
    rc = fs_move(msg_str(msg, 2), tmpfpath);
    if (rc)
    {
        eio("%s", fs_strerror(rc));
        goto cleanup;
    }
    ans = OK;

cleanup:
    parent_send(&parent, msg_ctx(msg, ans, filepath));
}

static void fn_scan_get_by_id(struct msg *msg)
{
    if (msg_check(msg, "si")) return;

    char const *ans = FAIL;
    char const *json = "";
    if (!api_scan_get_by_id(msg_int(msg, 1), &scan))
    {
        ans = OK;
        json = sched_dump_scan(&scan, (char *)buffer);
    }
    parent_send(&parent, msg_ctx(msg, ans, json));
}

static void fn_scan_get_by_job_id(struct msg *msg)
{
    if (msg_check(msg, "si")) return;

    char const *ans = FAIL;
    char const *json = "";
    if (!api_scan_get_by_job_id(msg_int(msg, 1), &scan))
    {
        ans = OK;
        json = sched_dump_scan(&scan, (char *)buffer);
    }
    parent_send(&parent, msg_ctx(msg, ans, json));
}

static void fn_scan_seq_count(struct msg *msg)
{
    if (msg_check(msg, "si")) return;

    char const *ans = FAIL;
    buffer[0] = '\0';

    unsigned count = 0;
    if (!api_scan_seq_count(msg_int(msg, 1), &count))
    {
        ans = OK;
        sprintf((char *)buffer, "%u", count);
    }
    parent_send(&parent, msg_ctx(msg, ans, buffer));
}

static void fn_scan_submit(struct msg *msg)
{
    if (msg_check(msg, "siiis")) return;

    char const *ans = FAIL;
    char const *json = "";

    long db_id = msg_int(msg, 1);
    long multi_hits = msg_int(msg, 2);
    long hmmer3_compat = msg_int(msg, 3);
    char const *path = msg_str(msg, 4);
    if (!api_scan_submit(db_id, multi_hits, hmmer3_compat, path, &job))
    {
        ans = OK;
        json = sched_dump_job(&job, (char *)buffer);
    }
    parent_send(&parent, msg_ctx(msg, ans, json));
}

static void fn_prods_file_up(struct msg *msg)
{
    if (msg_check(msg, "ss")) return;

    char const *ans = api_prods_file_up(msg_str(msg, 1)) ? FAIL : OK;
    parent_send(&parent, msg_ctx(msg, ans));
}

static void fn_prods_file_dl(struct msg *msg)
{
    if (msg_check(msg, "sis")) return;

    char const *ans = FAIL;
    char const *filepath = msg_str(msg, 2);
    static char tmpfpath[FILENAME_SIZE] = {0};

    int rc = fs_mkstemp(sizeof tmpfpath, tmpfpath);
    if (rc)
    {
        eio("%s", fs_strerror(rc));
        goto cleanup;
    }
    FILE *fp = fopen(tmpfpath, "wb");
    if (!fp)
    {
        eio("fopen failed");
        fs_unlink(tmpfpath);
        goto cleanup;
    }

    if (api_prods_file_dl(msg_int(msg, 1), fp))
    {
        fclose(fp);
        goto cleanup;
    }

    fclose(fp);
    rc = fs_move(msg_str(msg, 2), tmpfpath);
    if (rc)
    {
        eio("%s", fs_strerror(rc));
        goto cleanup;
    }
    ans = OK;

cleanup:
    parent_send(&parent, msg_ctx(msg, ans, filepath));
}

static enum rc dl_hmm(char const *filepath, void *data)
{
    long xxh3 = *((long *)data);
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

static enum rc dl_db(char const *filepath, void *data)
{
    long xxh3 = *((long *)data);
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
