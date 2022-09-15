#include "schedy_cmd.h"
#include "core/api.h"
#include "core/file.h"
#include "core/logging.h"
#include "core/sched_dump.h"
#include "core/xfile.h"
#include <string.h>

static inline char const *say_ok(void) { return "OK"; }

static inline char const *say_fail(void) { return "FAIL"; }

static inline char const *say_yes(void) { return "YES"; }

static inline char const *say_no(void) { return "NO"; }

#define error_parse() error("failed to parse command")

static enum rc download_hmm(char const *filepath, void *data);
static enum rc download_db(char const *filepath, void *data);
static bool encode_job_state(char const *str, enum sched_job_state *);

static char buffer[6 * 1024 * 1024] = {0};

static cmd_fn_t *schedy_cmds[] = {
#define X(_, A) &A,
    SCHEDY_CMD_MAP(X)
#undef X
};

static enum schedy_cmd parse(char const *cmd)
{
#define X(A, _)                                                                \
    if (!strcmp(cmd, STRINGIFY(A))) return SCHEDY_CMD_##A;
    SCHEDY_CMD_MAP(X)
#undef X
    return SCHEDY_CMD_INVALID;
}

cmd_fn_t *schedy_cmd(char const *cmd) { return schedy_cmds[parse(cmd)]; }

char const *schedy_cmd_invalid(struct cmd const *gc)
{
    (void)gc;
    eparse("invalid command");
    return say_fail();
}

char const *schedy_cmd_connect(struct cmd const *gc)
{
    if (!cmd_check(gc, "sss"))
    {
        error_parse();
        return say_fail();
    }
    if (api_init(gc->argv[1], gc->argv[2])) return say_fail();

    return say_ok();
}

char const *schedy_cmd_online(struct cmd const *gc)
{
    (void)gc;
    return api_is_reachable() ? say_yes() : say_no();
}

char const *schedy_cmd_wipe(struct cmd const *gc)
{
    (void)gc;
    return api_wipe() ? say_fail() : say_ok();
}

char const *schedy_cmd_hmm_up(struct cmd const *gc)
{
    static struct sched_hmm hmm = {0};

    if (!cmd_check(gc, "ss"))
    {
        error_parse();
        return say_fail();
    }

    return api_hmm_up(gc->argv[1], &hmm) ? say_fail() : say_ok();
}

char const *schedy_cmd_hmm_dl(struct cmd const *gc)
{
    if (!cmd_check(gc, "sis"))
    {
        error_parse();
        return say_fail();
    }

    int64_t xxh3 = cmd_get_i64(gc, 1);
    if (file_ensure_local(gc->argv[2], xxh3, &download_hmm, &xxh3))
        return say_fail();
    else
        return say_ok();
}

char const *schedy_cmd_hmm_get_by_id(struct cmd const *gc)
{
    static struct sched_hmm hmm = {0};

    if (!cmd_check(gc, "si"))
    {
        error_parse();
        return say_fail();
    }
    if (api_hmm_get_by_id(cmd_get_i64(gc, 1), &hmm)) return say_fail();
    return sched_dump_hmm(&hmm, (char *)buffer);
}

char const *schedy_cmd_hmm_get_by_xxh3(struct cmd const *gc)
{
    static struct sched_hmm hmm = {0};

    if (!cmd_check(gc, "si"))
    {
        error_parse();
        return say_fail();
    }
    if (api_hmm_get_by_xxh3(cmd_get_i64(gc, 1), &hmm)) return say_fail();
    return sched_dump_hmm(&hmm, (char *)buffer);
}

char const *schedy_cmd_hmm_get_by_job_id(struct cmd const *gc)
{
    static struct sched_hmm hmm = {0};

    if (!cmd_check(gc, "si"))
    {
        error_parse();
        return say_fail();
    }
    if (api_hmm_get_by_job_id(cmd_get_i64(gc, 1), &hmm)) return say_fail();
    return sched_dump_hmm(&hmm, (char *)buffer);
}

char const *schedy_cmd_hmm_get_by_filename(struct cmd const *gc)
{
    static struct sched_hmm hmm = {0};

    if (!cmd_check(gc, "ss"))
    {
        error_parse();
        return say_fail();
    }
    if (api_hmm_get_by_filename(gc->argv[1], &hmm)) return say_fail();
    return sched_dump_hmm(&hmm, (char *)buffer);
}

char const *schedy_cmd_db_up(struct cmd const *gc)
{
    static struct sched_db db = {0};

    if (!cmd_check(gc, "ss"))
    {
        error_parse();
        return say_fail();
    }
    return api_db_up(gc->argv[1], &db) ? say_fail() : say_ok();
}

char const *schedy_cmd_db_dl(struct cmd const *gc)
{
    if (!cmd_check(gc, "sis"))
    {
        error_parse();
        return say_fail();
    }

    int64_t xxh3 = cmd_get_i64(gc, 1);
    if (file_ensure_local(gc->argv[2], xxh3, &download_db, &xxh3))
        return say_fail();
    else
        return say_ok();
}

char const *schedy_cmd_db_get_by_id(struct cmd const *gc)
{
    static struct sched_db db = {0};

    if (!cmd_check(gc, "si"))
    {
        error_parse();
        return say_fail();
    }
    if (api_db_get_by_id(cmd_get_i64(gc, 1), &db)) return say_fail();
    return sched_dump_db(&db, (char *)buffer);
}

char const *schedy_cmd_db_get_by_xxh3(struct cmd const *gc)
{
    static struct sched_db db = {0};

    if (!cmd_check(gc, "si"))
    {
        error_parse();
        return say_fail();
    }
    if (api_db_get_by_xxh3(cmd_get_i64(gc, 1), &db)) return say_fail();
    return sched_dump_db(&db, (char *)buffer);
}

char const *schedy_cmd_db_get_by_hmm_id(struct cmd const *gc)
{
    static struct sched_db db = {0};

    if (!cmd_check(gc, "si"))
    {
        error_parse();
        return say_fail();
    }
    if (api_db_get_by_hmm_id(cmd_get_i64(gc, 1), &db)) return say_fail();
    return sched_dump_db(&db, (char *)buffer);
}

char const *schedy_cmd_db_get_by_filename(struct cmd const *gc)
{
    static struct sched_db db = {0};

    if (!cmd_check(gc, "ss"))
    {
        error_parse();
        return say_fail();
    }
    if (api_db_get_by_filename(gc->argv[1], &db)) return say_fail();
    return sched_dump_db(&db, (char *)buffer);
}

char const *schedy_cmd_job_next_pend(struct cmd const *gc)
{
    static struct sched_job job = {0};

    if (!cmd_check(gc, "s"))
    {
        error_parse();
        return say_fail();
    }
    if (api_job_next_pend(&job)) return say_fail();
    return sched_dump_job(&job, (char *)buffer);
}

char const *schedy_cmd_job_set_state(struct cmd const *gc)
{
    static char const empty[] = "";
    char const *msg = empty;
    if (gc->argc == 3)
    {
        if (!cmd_check(gc, "sis"))
        {
            error_parse();
            return say_fail();
        }
    }
    else
    {
        if (!cmd_check(gc, "siss"))
        {
            error_parse();
            return say_fail();
        }
        msg = gc->argv[3];
    }
    enum sched_job_state state = 0;
    if (!encode_job_state(gc->argv[2], &state)) return say_fail();
    int64_t job_id = cmd_get_i64(gc, 1);
    return api_job_set_state(job_id, state, msg) ? say_fail() : say_ok();
}

char const *schedy_cmd_job_inc_progress(struct cmd const *gc)
{
    if (!cmd_check(gc, "sii"))
    {
        error_parse();
        return say_fail();
    }
    int64_t job_id = cmd_get_i64(gc, 1);
    int64_t increment = cmd_get_i64(gc, 2);
    return api_job_inc_progress(job_id, increment) ? say_fail() : say_ok();
}

char const *schedy_cmd_scan_dl_seqs(struct cmd const *gc)
{
    static struct sched_scan scan = {0};
    static struct xfile_tmp file = {0};

    if (!cmd_check(gc, "si"))
    {
        error_parse();
        return say_fail();
    }

    enum rc rc = api_scan_get_by_id(cmd_get_i64(gc, 1), &scan);
    if (rc) return say_fail();

    if ((rc = xfile_tmp_open(&file))) return say_fail();

    if ((rc = api_scan_dl_seqs(scan.id, file.fp)))
    {
        xfile_tmp_del(&file);
        return say_fail();
    }
    return file.path;
}

char const *schedy_cmd_scan_get_by_job_id(struct cmd const *gc)
{
    static struct sched_scan scan = {0};

    if (!cmd_check(gc, "si"))
    {
        error_parse();
        return say_fail();
    }
    if (api_scan_get_by_job_id(cmd_get_i64(gc, 1), &scan)) return say_fail();
    return sched_dump_scan(&scan, (char *)buffer);
}

char const *schedy_cmd_scan_next_seq(struct cmd const *gc)
{
    (void)gc;
    return "";
}

char const *schedy_cmd_scan_seq_count(struct cmd const *gc)
{
    if (!cmd_check(gc, "si"))
    {
        error_parse();
        return say_fail();
    }
    unsigned count = 0;
    if (api_scan_seq_count(cmd_get_i64(gc, 1), &count)) return say_fail();
    sprintf((char *)buffer, "%u", count);
    return buffer;
}

char const *schedy_cmd_scan_submit(struct cmd const *gc)
{
    static struct sched_job job = {0};

    if (!cmd_check(gc, "siiis"))
    {
        error_parse();
        return say_fail();
    }
    int64_t db_id = cmd_get_i64(gc, 1);
    int64_t multi_hits = cmd_get_i64(gc, 2);
    int64_t hmmer3_compat = cmd_get_i64(gc, 3);
    if (api_scan_submit(db_id, multi_hits, hmmer3_compat, gc->argv[4], &job))
        return say_fail();
    return sched_dump_job(&job, (char *)buffer);
}

char const *schedy_cmd_prods_file_up(struct cmd const *gc)
{
    (void)gc;
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
