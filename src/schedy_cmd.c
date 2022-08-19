#include "schedy_cmd.h"
#include "deciphon/core/file.h"
#include "deciphon/core/getcmd.h"
#include "deciphon/core/logging.h"
#include "deciphon/sched/api.h"
#include "deciphon/sched/dump.h"

static inline void say_ok(void) { puts("OK"); }

static inline void say_fail(void) { puts("FAIL"); }

static inline void say_yes(void) { puts("YES"); }

static inline void say_no(void) { puts("NO"); }

#define error_parse() error("failed to parse command")

static unsigned char buffer[6 * 1024 * 1024] = {0};

void schedy_cmd_invalid(struct getcmd const *gc)
{
    (void)gc;
    error("invalid command");
    say_fail();
}

void schedy_cmd_connect(struct getcmd const *gc)
{
    if (!getcmd_check(gc, "sss"))
    {
        error_parse();
        say_fail();
    }
    else if (api_init(gc->argv[1], gc->argv[2]))
        say_fail();
    else
        say_ok();
}

void schedy_cmd_online(struct getcmd const *gc)
{
    if (api_is_reachable())
        say_yes();
    else
        say_no();
}

void schedy_cmd_disconnect(struct getcmd const *gc) {}

void schedy_cmd_wipe(struct getcmd const *gc)
{
    (void)gc;
    if (api_wipe())
        say_fail();
    else
        say_ok();
}

void schedy_cmd_hmm_up(struct getcmd const *gc)
{
    static struct sched_hmm hmm = {0};

    if (!getcmd_check(gc, "ss"))
    {
        error_parse();
        say_fail();
    }
    else if (api_up_hmm(gc->argv[1], &hmm))
        say_fail();
    else
        say_ok();
}

void schedy_cmd_hmm_dl(struct getcmd const *gc)
{
    if (!getcmd_check(gc, "sis"))
    {
        error_parse();
        say_fail();
        return;
    }

    int64_t xxh3 = getcmd_i64(gc, 1);
    if (file_ensure_local(gc->argv[2], xxh3, &download_hmm, &xxh3))
        say_fail();
    else
        say_ok();
}

void schedy_cmd_hmm_get_by_id(struct getcmd const *gc)
{
    static struct sched_hmm hmm = {0};

    if (!getcmd_check(gc, "si"))
    {
        error_parse();
        say_fail();
    }
    else if (api_get_hmm_by_id(getcmd_i64(gc, 1), &hmm))
        say_fail();
    else
        puts(sched_dump_hmm(&hmm, sizeof buffer, (char *)buffer));
}

void schedy_cmd_hmm_get_by_xxh3(struct getcmd const *gc)
{
    static struct sched_hmm hmm = {0};

    if (!getcmd_check(gc, "si"))
    {
        error_parse();
        say_fail();
    }
    else if (api_get_hmm_by_xxh3(getcmd_i64(gc, 1), &hmm))
        say_fail();
    else
        puts(sched_dump_hmm(&hmm, sizeof buffer, (char *)buffer));
}

void schedy_cmd_hmm_get_by_job_id(struct getcmd const *gc)
{
    static struct sched_hmm hmm = {0};

    if (!getcmd_check(gc, "si"))
    {
        error_parse();
        say_fail();
    }
    else if (api_get_hmm_by_job_id(getcmd_i64(gc, 1), &hmm))
        say_fail();
    else
        puts(sched_dump_hmm(&hmm, sizeof buffer, (char *)buffer));
}

void schedy_cmd_hmm_get_by_filename(struct getcmd const *gc)
{
    static struct sched_hmm hmm = {0};

    if (!getcmd_check(gc, "ss"))
    {
        error_parse();
        say_fail();
    }
    else if (api_get_hmm_by_filename(gc->argv[1], &hmm))
        say_fail();
    else
        puts(sched_dump_hmm(&hmm, sizeof buffer, (char *)buffer));
}

static enum rc download_hmm(char const *filepath, void *data);

void schedy_cmd_db_up(struct getcmd const *gc)
{
    static struct sched_db db = {0};

    if (!getcmd_check(gc, "ss"))
    {
        error_parse();
        say_fail();
    }
    else if (api_up_db(gc->argv[1], &db))
        say_fail();
    else
        say_ok();
}

void schedy_cmd_db_dl(struct getcmd const *gc) { (void)gc; }

void schedy_cmd_db_get_by_id(struct getcmd const *gc) { (void)gc; }
void schedy_cmd_db_get_by_xxh3(struct getcmd const *gc) { (void)gc; }
void schedy_cmd_db_get_by_job_id(struct getcmd const *gc) { (void)gc; }
void schedy_cmd_db_get_by_filename(struct getcmd const *gc) { (void)gc; }

void schedy_cmd_job_next_pend(struct getcmd const *gc) { (void)gc; }
void schedy_cmd_job_set_state(struct getcmd const *gc) { (void)gc; }
void schedy_cmd_job_inc_progress(struct getcmd const *gc) { (void)gc; }

void schedy_cmd_prods_file_up(struct getcmd const *gc) { (void)gc; }

void schedy_cmd_scan_next_seq(struct getcmd const *gc) { (void)gc; }
void schedy_cmd_scan_num_seqs(struct getcmd const *gc) { (void)gc; }
void schedy_cmd_scan_get_by_job_id(struct getcmd const *gc) { (void)gc; }

static enum rc download_hmm(char const *filepath, void *data)
{
    static struct sched_hmm hmm = {0};

    int64_t xxh3 = *((int64_t *)data);
    enum rc rc = api_get_hmm_by_xxh3(xxh3, &hmm);
    if (rc) return rc;

    FILE *fp = fopen(filepath, "wb");
    if (!fp) return eio("fopen");
    if ((rc = api_dl_hmm(hmm.id, fp)))
    {
        fclose(fp);
        return rc;
    }
    return fclose(fp) ? eio("fclose") : RC_OK;
}
