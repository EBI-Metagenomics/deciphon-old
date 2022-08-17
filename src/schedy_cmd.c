#include "schedy_cmd.h"
#include "deciphon/core/file.h"
#include "deciphon/core/getcmd.h"
#include "deciphon/core/logging.h"
#include "deciphon/sched/api.h"

static inline void say_ok(void) { puts("OK"); }

static inline void say_fail(void) { puts("FAIL"); }

static inline void say_yes(void) { puts("YES"); }

static inline void say_no(void) { puts("NO"); }

#define error_parse() error("failed to parse command")

void schedy_cmd_invalid(struct getcmd const *gc)
{
    (void)gc;
    error("invalid command");
    say_fail();
}

void schedy_cmd_init(struct getcmd const *gc)
{
    if (!getcmd_check(gc, "ss"))
    {
        error_parse();
        say_fail();
    }
    else if (api_init(gc->argv[1], gc->argv[2]))
        say_fail();
    else
        say_ok();
}

void schedy_cmd_is_reachable(struct getcmd const *gc)
{
    (void)gc;
    if (api_is_reachable())
        say_yes();
    else
        say_no();
}

void schedy_cmd_wipe(struct getcmd const *gc)
{
    (void)gc;
    if (api_wipe())
        say_fail();
    else
        say_ok();
}

void schedy_cmd_upload_hmm(struct getcmd const *gc)
{
    static struct sched_hmm hmm = {0};

    if (api_upload_hmm(gc->argv[1], &hmm))
        say_fail();
    else
        say_ok();
}

void schedy_cmd_get_hmm(struct getcmd const *gc)
{
    static struct sched_hmm hmm = {0};

    if (!getcmd_check(gc, "i"))
    {
        error_parse();
        say_fail();
    }
    else if (api_get_hmm(getcmd_i64(gc, 1), &hmm))
        say_fail();
    else
        say_ok();
}

static enum rc download_hmm(char const *filepath, void *data)
{
    static struct sched_hmm hmm = {0};

    int64_t xxh3 = *((int64_t *)data);
    enum rc rc = api_get_hmm_by_xxh3(xxh3, &hmm);

    FILE *fp = fopen(filepath, "wb");
    if (!fp) return eio("fopen");
    // enum rc rc = api_download_hmm_by_xxh3(id, fp);
    if (rc)
    {
        fclose(fp);
        return rc;
    }
    return fclose(fp) ? eio("fclose") : RC_OK;
}

void schedy_cmd_download_hmm(struct getcmd const *gc)
{
    if (!getcmd_check(gc, "is"))
    {
        error_parse();
        say_fail();
        return;
    }
    file_ensure_local(gc->argv[2], getcmd_i64(gc, 1), &download_hmm, 0);

    if ((rc = api_download_hmm(getcmd_i64(gc, 1), fp)))
    {
        error(RC_STRING(rc));
        say_fail();
    }
    else
        say_ok();
}
