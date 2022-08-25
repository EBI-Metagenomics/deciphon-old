#include "schedy_cmd.h"
#include "deciphon/core/file.h"
#include "deciphon/core/getcmd.h"
#include "deciphon/core/logging.h"
#include "deciphon/sched/api.h"
#include "deciphon/sched/dump.h"
#include "deciphon/sched/job_state.h"

static inline char const *say_ok(void) { return "OK"; }

static inline char const *say_fail(void) { return "FAIL"; }

static inline char const *say_yes(void) { return "YES"; }

static inline char const *say_no(void) { return "NO"; }

#define error_parse() error("failed to parse command")

static enum rc download_hmm(char const *filepath, void *data);
static enum rc download_db(char const *filepath, void *data);

static unsigned char buffer[6 * 1024 * 1024] = {0};

char const *schedy_cmd_invalid(struct getcmd const *gc)
{
    (void)gc;
    eparse("invalid command");
    return say_fail();
}

char const *schedy_cmd_connect(struct getcmd const *gc)
{
    if (!getcmd_check(gc, "sss"))
    {
        error_parse();
        return say_fail();
    }
    if (api_init(gc->argv[1], gc->argv[2])) return say_fail();

    return say_ok();
}

char const *schedy_cmd_online(struct getcmd const *gc)
{
    (void)gc;
    return api_is_reachable() ? say_yes() : say_no();
}

char const *schedy_cmd_wipe(struct getcmd const *gc)
{
    (void)gc;
    return api_wipe() ? say_fail() : say_ok();
}

char const *schedy_cmd_hmm_up(struct getcmd const *gc)
{
    static struct sched_hmm hmm = {0};

    if (!getcmd_check(gc, "ss"))
    {
        error_parse();
        return say_fail();
    }

    return api_hmm_up(gc->argv[1], &hmm) ? say_fail() : say_ok();
}

char const *schedy_cmd_hmm_dl(struct getcmd const *gc)
{
    if (!getcmd_check(gc, "sis"))
    {
        error_parse();
        return say_fail();
    }

    int64_t xxh3 = getcmd_i64(gc, 1);
    if (file_ensure_local(gc->argv[2], xxh3, &download_hmm, &xxh3))
        return say_fail();
    else
        return say_ok();
}

char const *schedy_cmd_hmm_get_by_id(struct getcmd const *gc)
{
    static struct sched_hmm hmm = {0};

    if (!getcmd_check(gc, "si"))
    {
        error_parse();
        return say_fail();
    }
    if (api_hmm_get_by_id(getcmd_i64(gc, 1), &hmm)) return say_fail();
    return sched_dump_hmm(&hmm, sizeof buffer, (char *)buffer);
}

char const *schedy_cmd_hmm_get_by_xxh3(struct getcmd const *gc)
{
    static struct sched_hmm hmm = {0};

    if (!getcmd_check(gc, "si"))
    {
        error_parse();
        return say_fail();
    }
    if (api_hmm_get_by_xxh3(getcmd_i64(gc, 1), &hmm)) return say_fail();
    return sched_dump_hmm(&hmm, sizeof buffer, (char *)buffer);
}

char const *schedy_cmd_hmm_get_by_job_id(struct getcmd const *gc)
{
    static struct sched_hmm hmm = {0};

    if (!getcmd_check(gc, "si"))
    {
        error_parse();
        return say_fail();
    }
    if (api_hmm_get_by_job_id(getcmd_i64(gc, 1), &hmm)) return say_fail();
    return sched_dump_hmm(&hmm, sizeof buffer, (char *)buffer);
}

char const *schedy_cmd_hmm_get_by_filename(struct getcmd const *gc)
{
    static struct sched_hmm hmm = {0};

    if (!getcmd_check(gc, "ss"))
    {
        error_parse();
        return say_fail();
    }
    if (api_hmm_get_by_filename(gc->argv[1], &hmm)) return say_fail();
    return sched_dump_hmm(&hmm, sizeof buffer, (char *)buffer);
}

char const *schedy_cmd_db_up(struct getcmd const *gc)
{
    static struct sched_db db = {0};

    if (!getcmd_check(gc, "ss"))
    {
        error_parse();
        return say_fail();
    }
    return api_db_up(gc->argv[1], &db) ? say_fail() : say_ok();
}

char const *schedy_cmd_db_dl(struct getcmd const *gc)
{
    if (!getcmd_check(gc, "sis"))
    {
        error_parse();
        return say_fail();
    }

    int64_t xxh3 = getcmd_i64(gc, 1);
    if (file_ensure_local(gc->argv[2], xxh3, &download_db, &xxh3))
        return say_fail();
    else
        return say_ok();
}

char const *schedy_cmd_db_get_by_id(struct getcmd const *gc)
{
    static struct sched_db db = {0};

    if (!getcmd_check(gc, "si"))
    {
        error_parse();
        return say_fail();
    }
    if (api_db_get_by_id(getcmd_i64(gc, 1), &db)) return say_fail();
    return sched_dump_db(&db, sizeof buffer, (char *)buffer);
}
char const *schedy_cmd_db_get_by_xxh3(struct getcmd const *gc)
{
    static struct sched_db db = {0};

    if (!getcmd_check(gc, "si"))
    {
        error_parse();
        return say_fail();
    }
    if (api_db_get_by_xxh3(getcmd_i64(gc, 1), &db)) return say_fail();
    return sched_dump_db(&db, sizeof buffer, (char *)buffer);
}
char const *schedy_cmd_db_get_by_hmm_id(struct getcmd const *gc)
{
    static struct sched_db db = {0};

    if (!getcmd_check(gc, "si"))
    {
        error_parse();
        return say_fail();
    }
    if (api_db_get_by_hmm_id(getcmd_i64(gc, 1), &db)) return say_fail();
    return sched_dump_db(&db, sizeof buffer, (char *)buffer);
}
char const *schedy_cmd_db_get_by_filename(struct getcmd const *gc)
{
    static struct sched_db db = {0};

    if (!getcmd_check(gc, "ss"))
    {
        error_parse();
        return say_fail();
    }
    if (api_db_get_by_filename(gc->argv[1], &db)) return say_fail();
    return sched_dump_db(&db, sizeof buffer, (char *)buffer);
}

char const *schedy_cmd_job_next_pend(struct getcmd const *gc)
{
    static struct sched_job job = {0};

    if (!getcmd_check(gc, "s"))
    {
        error_parse();
        return say_fail();
    }
    if (api_job_next_pend(&job)) return say_fail();
    return sched_dump_job(&job, sizeof buffer, (char *)buffer);
}
char const *schedy_cmd_job_set_state(struct getcmd const *gc)
{
    char const *msg = 0;
    if (gc->argc == 3)
    {
        if (!getcmd_check(gc, "sis"))
        {
            error_parse();
            return say_fail();
        }
        msg = "";
    }
    else
    {
        if (!getcmd_check(gc, "siss"))
        {
            error_parse();
            return say_fail();
        }
        msg = gc->argv[3];
    }
    enum sched_job_state state = job_state_encode(gc->argv[2]);
    int64_t job_id = getcmd_i64(gc, 1);
    return api_job_set_state(job_id, state, msg) ? say_fail() : say_ok();
}

char const *schedy_cmd_job_inc_progress(struct getcmd const *gc)
{
    if (!getcmd_check(gc, "sii"))
    {
        error_parse();
        return say_fail();
    }
    int64_t job_id = getcmd_i64(gc, 1);
    int64_t increment = getcmd_i64(gc, 2);
    return api_job_inc_progress(job_id, increment) ? say_fail() : say_ok();
}

char const *schedy_cmd_prods_file_up(struct getcmd const *gc)
{
    (void)gc;
    return "";
}

char const *schedy_cmd_scan_next_seq(struct getcmd const *gc)
{
    (void)gc;
    return "";
}
char const *schedy_cmd_scan_num_seqs(struct getcmd const *gc)
{
    (void)gc;
    return "";
}
char const *schedy_cmd_scan_get_by_job_id(struct getcmd const *gc)
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
