#include "api.h"
#include "argless.h"
#include "array_size.h"
#include "cmd.h"
#include "core/sched_dump.h"
#include "die.h"
#include "file.h"
#include "fs.h"
#include "lazylog.h"
#include "logy.h"
#include "loop/global.h"
#include "loop/parent.h"
#include "msg.h"
#include "pidfile.h"
#include "stringify.h"
#include <stdlib.h>
#include <string.h>

#define API_URL "http://127.0.0.1:49329"
#define API_KEY "change-me"

static struct argl_option const options[] = {
    {"url", 'u', ARGL_TEXT("APIURL", API_URL), "API url."},
    {"key", 'k', ARGL_TEXT("APIKEY", API_KEY), "API key."},
    {"loglevel", 'L', ARGL_TEXT("LOGLEVEL", "0"), "Logging level."},
    {"pid", 'p', ARGL_TEXT("PIDFILE", ARGL_NULL), "PID file."},
    ARGL_DEFAULT,
    ARGL_END,
};

static struct argl argl = {.options = options,
                           .args_doc = NULL,
                           .doc = "Schedy program.",
                           .version = "1.0.0"};

static bool linger(void);
static void cleanup(void);

static void on_read(char *line);

int main(int argc, char *argv[])
{
    argl_parse(&argl, argc, argv);
    if (argl_nargs(&argl)) argl_usage(&argl);
    if (argl_has(&argl, "pid")) pidfile_save(argl_get(&argl, "pid"));
    int loglvl = argl_get(&argl, "loglevel")[0] - '0';
    char const *url = argl_get(&argl, "url");
    char const *key = argl_get(&argl, "key");

    global_init(argv[0], loglvl);
    global_linger_setup(&linger, &cleanup);
    parent_init(&on_read, &global_shutdown, &global_shutdown);
    if (api_init(url, key)) die();
    return global_run();
}

#define CMD_MAP(X)                                                             \
    X(quit, "")                                                                \
    X(help, "")                                                                \
    X(echo, "[...]")                                                           \
    X(online, "")                                                              \
    X(wipe, "")                                                                \
    X(cancel, "")                                                              \
                                                                               \
    X(hmm_up, "HMM_FILE")                                                      \
    X(hmm_dl, "XXH3 OUTPUT_FILE")                                              \
    X(hmm_get_by_id, "HMM_ID")                                                 \
    X(hmm_get_by_xxh3, "XXH3")                                                 \
    X(hmm_get_by_job_id, "JOB_ID")                                             \
    X(hmm_get_by_filename, "FILENAME")                                         \
                                                                               \
    X(db_up, "DB_FILE")                                                        \
    X(db_dl, "XXH3 OUTPUT_FILE")                                               \
    X(db_get_by_id, "DB_ID")                                                   \
    X(db_get_by_xxh3, "XXH3")                                                  \
    X(db_get_by_hmm_id, "HMM_ID")                                              \
    X(db_get_by_filename, "FILENAME")                                          \
                                                                               \
    X(job_next_pend, "")                                                       \
    X(job_set_state, "JOB_ID STATE [MSG]")                                     \
    X(job_inc_progress, "JOB_ID PROGRESS")                                     \
    X(job_get_by_id, "JOB_ID")                                                 \
                                                                               \
    X(scan_dl_seqs, "SCAN_ID FILE")                                            \
    X(scan_get_by_id, "SCAN_ID")                                               \
    X(scan_get_by_job_id, "JOB_ID")                                            \
    X(scan_seq_count, "SCAN_ID")                                               \
    X(scan_submit, "DB_ID MULTI_HITS HMMER3_COMPAT FASTA_FILE")                \
                                                                               \
    X(prods_file_up, "PRODS_FILE")                                             \
    X(prods_file_dl, "SCAN_ID FILE")

#define X(A, _) static void A(struct msg *);
CMD_MAP(X)
#undef X

static struct cmd_entry entries[] = {
#define X(A, B) {&A, stringify(A), B},
    CMD_MAP(X)
#undef X
};

static void on_read(char *line)
{
    static struct msg msg = {0};
    if (msg_parse(&msg, line)) return;
    struct cmd_entry *e = cmd_find(array_size(entries), entries, msg_cmd(&msg));
    if (!e)
    {
        einval("unrecognized command: %s", msg_cmd(&msg));
        return;
    }

    msg_shift(&msg);
    (*e->func)(&msg);
}

static bool linger(void) { return parent_closed(); }

static void cleanup(void)
{
    parent_close();
    api_cleanup();
}

static enum rc dl_hmm(char const *filepath, void *data);
static enum rc dl_db(char const *filepath, void *data);
static bool encode_job_state(char const *str, enum sched_job_state *);

static char buffer[6 * 1024 * 1024] = {0};
static struct sched_hmm hmm = {0};
static struct sched_db db = {0};
static struct sched_job job = {0};
enum sched_job_state state = 0;
static struct sched_scan scan = {0};

static void quit(struct msg *msg)
{
    unused(msg);
    global_shutdown();
}

static void echo(struct msg *msg) { parent_send(msg_unparse(msg)); }

static void help(struct msg *msg)
{
    unused(msg);
    cmd_help_init();

    for (size_t i = 0; i < array_size(entries); ++i)
        cmd_help_add(entries[i].name, entries[i].doc);

    parent_send(cmd_help_table());
}

static void online(struct msg *msg)
{
    char const *ans = api_is_reachable() ? "yes" : "no";
    parent_send(msg_ctx(msg, ans));
}

static void wipe(struct msg *msg)
{
    char const *ans = api_wipe() ? "fail" : "ok";
    parent_send(msg_ctx(msg, ans));
}

static void cancel(struct msg *msg)
{
    unused(msg);
    debug("not implemented yet");
}

static void hmm_up(struct msg *msg)
{
    if (msg_check(msg, "s")) return;
    char const *ans = api_hmm_up(msg_str(msg, 0), &hmm) ? "fail" : "ok";
    parent_send(msg_ctx(msg, ans));
}

static void hmm_dl(struct msg *msg)
{
    if (msg_check(msg, "is")) return;
    long xxh3 = msg_int(msg, 0);
    char const *name = msg_str(msg, 1);
    char const *ans =
        file_ensure_local(name, xxh3, &dl_hmm, &xxh3) ? "fail" : "ok";
    parent_send(msg_ctx(msg, ans));
}

static void hmm_get_by_id(struct msg *msg)
{
    if (msg_check(msg, "i")) return;

    char const *ans = "fail";
    char const *json = "";
    if (!api_hmm_get_by_id(msg_int(msg, 0), &hmm))
    {
        ans = "ok";
        json = sched_dump_hmm(&hmm, (char *)buffer);
    }
    parent_send(msg_ctx(msg, ans, json));
}

static void hmm_get_by_xxh3(struct msg *msg)
{
    if (msg_check(msg, "i")) return;

    char const *ans = "fail";
    char const *json = "";
    if (!api_hmm_get_by_xxh3(msg_int(msg, 0), &hmm))
    {
        ans = "ok";
        json = sched_dump_hmm(&hmm, (char *)buffer);
    }
    parent_send(msg_ctx(msg, ans, json));
}

static void hmm_get_by_job_id(struct msg *msg)
{
    if (msg_check(msg, "i")) return;

    char const *ans = "fail";
    char const *json = "";
    if (!api_hmm_get_by_job_id(msg_int(msg, 0), &hmm))
    {
        ans = "ok";
        json = sched_dump_hmm(&hmm, (char *)buffer);
    }
    parent_send(msg_ctx(msg, ans, json));
}

static void hmm_get_by_filename(struct msg *msg)
{
    if (msg_check(msg, "s")) return;

    char const *ans = "fail";
    char const *json = "";
    if (!api_hmm_get_by_filename(msg_str(msg, 0), &hmm))
    {
        ans = "ok";
        json = sched_dump_hmm(&hmm, (char *)buffer);
    }
    parent_send(msg_ctx(msg, ans, json));
}

static void db_up(struct msg *msg)
{
    if (msg_check(msg, "s")) return;

    char const *ans = api_db_up(msg_str(msg, 0), &db) ? "fail" : "ok";
    parent_send(msg_ctx(msg, ans));
}

static void db_dl(struct msg *msg)
{
    if (msg_check(msg, "is")) return;

    char const *ans = "fail";
    long xxh3 = msg_int(msg, 0);
    char const *name = msg_str(msg, 1);
    if (!file_ensure_local(name, xxh3, &dl_db, &xxh3)) ans = "ok";

    parent_send(msg_ctx(msg, ans));
}

static void db_get_by_id(struct msg *msg)
{
    if (msg_check(msg, "i")) return;

    char const *ans = "fail";
    char const *json = "";
    if (!api_db_get_by_id(msg_int(msg, 0), &db))
    {
        ans = "ok";
        json = sched_dump_db(&db, (char *)buffer);
    }
    parent_send(msg_ctx(msg, ans, json));
}

static void db_get_by_xxh3(struct msg *msg)
{
    if (msg_check(msg, "i")) return;

    char const *ans = "fail";
    char const *json = "";
    if (!api_db_get_by_xxh3(msg_int(msg, 0), &db))
    {
        ans = "ok";
        json = sched_dump_db(&db, (char *)buffer);
    }
    parent_send(msg_ctx(msg, ans, json));
}

static void db_get_by_hmm_id(struct msg *msg)
{
    if (msg_check(msg, "i")) return;

    char const *ans = "fail";
    char const *json = "";
    if (!api_db_get_by_hmm_id(msg_int(msg, 0), &db))
    {
        ans = "ok";
        json = sched_dump_db(&db, (char *)buffer);
    }
    parent_send(msg_ctx(msg, ans, json));
}

static void db_get_by_filename(struct msg *msg)
{
    if (msg_check(msg, "s")) return;

    char const *ans = "fail";
    char const *json = "";
    if (!api_db_get_by_filename(msg_str(msg, 0), &db))
    {
        ans = "ok";
        json = sched_dump_db(&db, (char *)buffer);
    }
    parent_send(msg_ctx(msg, ans, json));
}

static void job_next_pend(struct msg *msg)
{
    char const *ans = "fail";
    char const *json = "";
    enum rc rc = api_job_next_pend(&job);
    if (!rc || rc == RC_END)
    {
        ans = rc == RC_END ? "end" : "ok";
        json = rc != RC_END ? sched_dump_job(&job, (char *)buffer) : "";
    }
    parent_send(msg_ctx(msg, ans, json));
}

static void job_set_state(struct msg *msg)
{
    if (msg_argc(msg) == 2 && msg_check(msg, "is")) return;
    if (msg_check(msg, "is*")) return;

    char const *ans = "fail";
    char const *json = "";

    if (!encode_job_state(msg_str(msg, 1), &state)) goto cleanup;

    long job_id = msg_int(msg, 0);
    char const *error = msg_argc(msg) == 2 ? "" : msg_str(msg, 2);
    if (api_job_set_state(job_id, state, error)) goto cleanup;
    if (api_job_get_by_id(job_id, &job)) goto cleanup;
    ans = "ok";
    json = sched_dump_job(&job, (char *)buffer);

cleanup:
    parent_send(msg_ctx(msg, ans, json));
}

static void job_inc_progress(struct msg *msg)
{
    if (msg_check(msg, "ii")) return;

    long job_id = msg_int(msg, 0);
    long increment = msg_int(msg, 1);
    char const *ans = api_job_inc_progress(job_id, increment) ? "fail" : "ok";

    parent_send(msg_ctx(msg, ans));
}

static void job_get_by_id(struct msg *msg)
{
    if (msg_check(msg, "i")) return;

    char const *ans = "fail";
    char const *json = "";
    if (!api_job_get_by_id(msg_int(msg, 0), &job))
    {
        ans = "ok";
        json = sched_dump_job(&job, (char *)buffer);
    }
    parent_send(msg_ctx(msg, ans, json));
}

static void scan_dl_seqs(struct msg *msg)
{
    if (msg_check(msg, "is")) return;

    char const *ans = "fail";
    char const *filepath = msg_str(msg, 1);
    static char tmpfpath[FILENAME_SIZE] = {0};

    if (api_scan_get_by_id(msg_int(msg, 0), &scan)) goto cleanup;

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
    rc = fs_move(msg_str(msg, 1), tmpfpath);
    if (rc)
    {
        eio("%s", fs_strerror(rc));
        goto cleanup;
    }
    ans = "ok";

cleanup:
    parent_send(msg_ctx(msg, ans, filepath));
}

static void scan_get_by_id(struct msg *msg)
{
    if (msg_check(msg, "i")) return;

    char const *ans = "fail";
    char const *json = "";
    if (!api_scan_get_by_id(msg_int(msg, 0), &scan))
    {
        ans = "ok";
        json = sched_dump_scan(&scan, (char *)buffer);
    }
    parent_send(msg_ctx(msg, ans, json));
}

static void scan_get_by_job_id(struct msg *msg)
{
    if (msg_check(msg, "i")) return;

    char const *ans = "fail";
    char const *json = "";
    if (!api_scan_get_by_job_id(msg_int(msg, 0), &scan))
    {
        ans = "ok";
        json = sched_dump_scan(&scan, (char *)buffer);
    }
    parent_send(msg_ctx(msg, ans, json));
}

static void scan_seq_count(struct msg *msg)
{
    if (msg_check(msg, "i")) return;

    char const *ans = "fail";
    buffer[0] = '\0';

    unsigned count = 0;
    if (!api_scan_seq_count(msg_int(msg, 0), &count))
    {
        ans = "ok";
        sprintf((char *)buffer, "%u", count);
    }
    parent_send(msg_ctx(msg, ans, buffer));
}

static void scan_submit(struct msg *msg)
{
    if (msg_check(msg, "iiis")) return;

    char const *ans = "fail";
    char const *json = "";

    long db_id = msg_int(msg, 0);
    long multi_hits = msg_int(msg, 1);
    long hmmer3_compat = msg_int(msg, 2);
    char const *path = msg_str(msg, 3);
    if (!api_scan_submit(db_id, multi_hits, hmmer3_compat, path, &job))
    {
        ans = "ok";
        json = sched_dump_job(&job, (char *)buffer);
    }
    parent_send(msg_ctx(msg, ans, json));
}

static void prods_file_up(struct msg *msg)
{
    if (msg_check(msg, "s")) return;

    char const *ans = api_prods_file_up(msg_str(msg, 0)) ? "fail" : "ok";
    parent_send(msg_ctx(msg, ans));
}

static void prods_file_dl(struct msg *msg)
{
    if (msg_check(msg, "is")) return;

    char const *ans = "fail";
    char const *filepath = msg_str(msg, 1);
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

    if (api_prods_file_dl(msg_int(msg, 0), fp))
    {
        fclose(fp);
        goto cleanup;
    }

    fclose(fp);
    rc = fs_move(msg_str(msg, 1), tmpfpath);
    if (rc)
    {
        eio("%s", fs_strerror(rc));
        goto cleanup;
    }
    ans = "ok";

cleanup:
    parent_send(msg_ctx(msg, ans, filepath));
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
