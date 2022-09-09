#include "deciphon/api.h"
#include "core/mime.h"
#include "core/xcurl.h"
#include "deciphon/core/buff.h"
#include "deciphon/core/compiler.h"
#include "deciphon/core/http.h"
#include "deciphon/core/logging.h"
#include "deciphon/core/xfile.h"
#include "deciphon/core/xmath.h"
#include "deciphon/sched.h"
#include "jx.h"
#include "sched/count.h"
#include <inttypes.h>
#include <stdarg.h>
#include <string.h>

static char filename[SCHED_FILENAME_SIZE] = {0};
static thread_local struct api_error api_err = {0};

static char _query[128] = {0};

static char request[1024] = {0};

static JR_DECLARE(jr, 128);

#define eapi(x)                                                                \
    ({                                                                         \
        error(" api_rc[%d] %s", (x).rc, (x).msg);                              \
        RC_EAPI;                                                               \
    })

static inline void reset_api_error(void)
{
    api_err.rc = 0;
    api_err.msg[0] = 0;
}

static enum rc parse_api_error(struct api_error *e, struct jr jr[]);

static inline enum rc parse_json(void)
{
    return jr_parse(jr, xcurl_body_size(), xcurl_body_data()) ? RC_EFAIL
                                                              : RC_OK;
}

static char const *query(char const *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    vsnprintf(_query, sizeof _query, fmt, args);
    va_end(args);

    return _query;
}

enum rc api_init(char const *url_stem, char const *api_key)
{
    JR_INIT(jr);
    enum rc rc = xcurl_init(url_stem, api_key);
    if (rc) return rc;

    return RC_OK;
}

void api_cleanup(void) { xcurl_cleanup(); }

struct api_error const *api_error(void) { return &api_err; }

static inline enum rc get(char const *query, long *http)
{
    return xcurl_get(query, http);
}

static inline bool recognized_http_status(long http_status)
{
    return http_status == 401 || http_status == 404 || http_status == 406 ||
           http_status == 418 || http_status == 422;
}

bool api_is_reachable(void)
{
    bool reachable = false;
    long http = 0;
    if (get("/", &http)) goto cleanup;

    reachable = http == 200;

cleanup:
    return reachable;
}

enum rc api_wipe(void)
{
    long http = 0;
    enum rc rc = xcurl_delete("/sched/wipe", &http);
    if (rc) goto cleanup;

    if (http != 200) rc = ehttp(http_status_string(http));

cleanup:
    return rc;
}

static inline enum rc upload(char const *query, long *http,
                             struct mime_file const *mime, char const *filepath)
{
    // body_reset(response);
    return xcurl_upload(query, http, mime, filepath);
}

enum rc api_hmm_up(char const *filepath, struct sched_hmm *hmm)
{
    reset_api_error();

    xfile_basename(filename, filepath, sizeof filename);
    MIME_FILE_DEF(mime, "hmm_file", filename, "text/plain");

    long http = 0;
    enum rc rc = upload("/hmms/", &http, &mime, filepath);
    if (rc) goto cleanup;

    if ((rc = parse_json())) goto cleanup;

    if (http == 201)
    {
        sched_hmm_init(hmm);
        rc = sched_hmm_parse(hmm, jr);
    }
    else if (recognized_http_status(http))
    {
        if (!(rc = parse_api_error(&api_err, jr)))
        {
            rc = eapi(api_err);
        }
    }
    else
    {
        rc = ehttp(http_status_string(http));
    }

cleanup:
    return rc;
}

enum rc api_hmm_dl(int64_t id, FILE *fp)
{
    reset_api_error();

    enum rc rc = RC_OK;

    long http = 0;
    rc = xcurl_download(query("/hmms/%" PRId64 "/download", id), &http, fp);
    if (rc) goto cleanup;

    if (http == 200)
    {
        rc = RC_OK;
    }
    else if (recognized_http_status(http))
    {
        if (!(rc = parse_api_error(&api_err, jr)))
        {
            rc = eapi(api_err);
        }
    }
    else
    {
        rc = ehttp(http_status_string(http));
    }

cleanup:
    return rc;
}

static inline enum rc patch(char const *query, long *http)
{
    // body_reset(response);
    return xcurl_patch(query, http, request);
}

union param
{
    int64_t i;
    char const *s;
};

enum param_type
{
    HMM_ID,
    DB_ID,
    XXH3,
    JOB_ID,
    FILENAME,
};

static enum rc get_hmm_by(struct sched_hmm *, union param, enum param_type);

enum rc api_hmm_get_by_id(int64_t id, struct sched_hmm *hmm)
{
    return get_hmm_by(hmm, (union param){.i = id}, HMM_ID);
}

enum rc api_hmm_get_by_xxh3(int64_t xxh3, struct sched_hmm *hmm)
{
    return get_hmm_by(hmm, (union param){.i = xxh3}, XXH3);
}

enum rc api_hmm_get_by_job_id(int64_t job_id, struct sched_hmm *hmm)
{
    return get_hmm_by(hmm, (union param){.i = job_id}, JOB_ID);
}

enum rc api_hmm_get_by_filename(char const *filename, struct sched_hmm *hmm)
{
    return get_hmm_by(hmm, (union param){.s = filename}, FILENAME);
}

static enum rc get_hmm_by(struct sched_hmm *hmm, union param p,
                          enum param_type type)
{
    sched_hmm_init(hmm);
    reset_api_error();

    long http = 0;
    enum rc rc = RC_OK;

    if (type == HMM_ID) rc = get(query("/hmms/%" PRId64, p.i), &http);
    if (type == XXH3) rc = get(query("/hmms/xxh3/%" PRId64, p.i), &http);
    if (type == JOB_ID) rc = get(query("/jobs/job_id/%" PRId64, p.i), &http);
    if (type == FILENAME) rc = get(query("/hmms/filename/%s", p.s), &http);

    if (rc) goto cleanup;

    if ((rc = parse_json())) goto cleanup;

    if (http == 200)
    {
        rc = sched_hmm_parse(hmm, jr);
    }
    else if (recognized_http_status(http))
    {
        if (!(rc = parse_api_error(&api_err, jr)))
        {
            rc = eapi(api_err);
        }
    }
    else
    {
        rc = ehttp(http_status_string(http));
    }

cleanup:
    return rc;
}

enum rc api_db_up(char const *filepath, struct sched_db *db)
{
    reset_api_error();
    sched_db_init(db);

    MIME_FILE_DEF(mime, "db_file", filename, "application/octet-stream");

    long http = 0;
    enum rc rc = upload("/dbs/", &http, &mime, filepath);
    if (rc) goto cleanup;

    if ((rc = parse_json())) goto cleanup;

    if (http == 201)
    {
        rc = sched_db_parse(db, jr);
    }
    else if (recognized_http_status(http))
    {
        if (!(rc = parse_api_error(&api_err, jr)))
        {
            rc = eapi(api_err);
        }
    }
    else
    {
        rc = ehttp(http_status_string(http));
    }

cleanup:
    return rc;
}

enum rc api_db_dl(int64_t id, FILE *fp)
{
    enum rc rc = RC_OK;

    long http = 0;
    rc = xcurl_download(query("/dbs/%" PRId64 "/download", id), &http, fp);
    if (rc) goto cleanup;

    if (http == 200)
    {
        rc = RC_OK;
    }
    else if (recognized_http_status(http))
    {
        if (!(rc = parse_api_error(&api_err, jr)))
        {
            rc = eapi(api_err);
        }
    }
    else
    {
        rc = ehttp(http_status_string(http));
    }

cleanup:
    return rc;
}

static enum rc get_db_by(struct sched_db *db, union param p,
                         enum param_type type);

enum rc api_db_get_by_id(int64_t id, struct sched_db *db)
{
    return get_db_by(db, (union param){.i = id}, DB_ID);
}

enum rc api_db_get_by_xxh3(int64_t xxh3, struct sched_db *db)
{
    return get_db_by(db, (union param){.i = xxh3}, XXH3);
}

enum rc api_db_get_by_hmm_id(int64_t hmm_id, struct sched_db *db)
{
    return get_db_by(db, (union param){.i = hmm_id}, HMM_ID);
}

enum rc api_db_get_by_filename(char const *filename, struct sched_db *db)
{
    return get_db_by(db, (union param){.s = filename}, FILENAME);
}

static enum rc get_db_by(struct sched_db *db, union param p,
                         enum param_type type)
{
    sched_db_init(db);
    reset_api_error();

    long http = 0;
    enum rc rc = RC_OK;

    if (type == DB_ID)
        rc = get(query("/dbs/%" PRId64 "?id_type=db_id", p.i), &http);
    if (type == XXH3)
        rc = get(query("/dbs/%" PRId64 "?id_type=xxh3", p.i), &http);
    if (type == JOB_ID)
        rc = get(query("/jobs/%" PRId64 "/db&id_type=job_id", p.i), &http);
    if (type == HMM_ID)
        rc = get(query("/jobs/%" PRId64 "/db&id_type=hmm_id", p.i), &http);
    if (type == FILENAME)
        rc = get(query("/dbs/%s?id_type=filename", p.s), &http);

    if (rc) goto cleanup;

    if ((rc = parse_json())) goto cleanup;

    if (http == 200)
    {
        rc = sched_db_parse(db, jr);
    }
    else if (recognized_http_status(http))
    {
        if (!(rc = parse_api_error(&api_err, jr)))
        {
            rc = eapi(api_err);
        }
    }
    else
    {
        rc = ehttp(http_status_string(http));
    }

cleanup:
    return rc;
}

enum rc api_job_next_pend(struct sched_job *job)
{
    sched_job_init(job);
    reset_api_error();

    enum rc rc = RC_OK;

    long http = 0;
    if ((rc = get("/jobs/next_pend", &http))) goto cleanup;

    if ((rc = parse_json())) goto cleanup;

    if (http == 200)
    {
        rc = sched_job_parse(job, jr);
    }
    else if (http == 404)
    {
        if (!(rc = parse_api_error(&api_err, jr)))
        {
            if (api_err.rc == 5)
                rc = RC_END;
            else
                rc = eapi(api_err);
        }
    }
    else if (recognized_http_status(http))
    {
        if (!(rc = parse_api_error(&api_err, jr)))
        {
            rc = eapi(api_err);
        }
    }
    else
    {
        rc = ehttp(http_status_string(http));
    }

cleanup:
    return rc;
}

static char const job_states[][5] = {[SCHED_PEND] = "pend",
                                     [SCHED_RUN] = "run",
                                     [SCHED_DONE] = "done",
                                     [SCHED_FAIL] = "fail"};

enum rc set_job_state(int64_t job_id, enum sched_job_state state,
                      char const *state_error, long *http)
{
    char *p = request;
    p += jw_object_open(p);

    p += jw_string(p, "job_id");
    p += jw_colon(p);
    p += jw_long(p, job_id);

    p += jw_comma(p);

    p += jw_string(p, "state");
    p += jw_colon(p);
    p += jw_string(p, job_states[state]);

    p += jw_comma(p);

    p += jw_string(p, "error");
    p += jw_colon(p);
    p += jw_string(p, state_error);

    p += jw_object_close(p);
    *p = 0;

    return patch(query("/jobs/%" PRId64 "/state", job_id), http);
}

enum rc api_job_set_state(int64_t job_id, enum sched_job_state state,
                          char const *msg)
{
    reset_api_error();

    long http = 0;
    enum rc rc = set_job_state(job_id, state, msg, &http);
    if (rc) goto cleanup;

    if ((rc = parse_json())) goto cleanup;

    if (http == 200)
    {
        rc = RC_OK;
    }
    else if (recognized_http_status(http))
    {
        if (!(rc = parse_api_error(&api_err, jr)))
        {
            rc = eapi(api_err);
        }
    }
    else
    {
        rc = ehttp(http_status_string(http));
    }

cleanup:
    return rc;
}

enum rc api_job_inc_progress(int64_t job_id, int increment)
{
    reset_api_error();

    char *p = request;
    p += jw_object_open(p);
    p += jw_string(p, "increment");
    p += jw_colon(p);
    p += jw_long(p, increment);
    p += jw_object_close(p);
    *p = 0;

    long http = 0;
    enum rc rc = patch(query("/jobs/%" PRId64 "/progress", job_id), &http);
    if (rc) goto cleanup;

    if ((rc = parse_json())) goto cleanup;

    if (http == 200)
    {
        rc = RC_OK;
    }
    else if (recognized_http_status(http))
    {
        if (!(rc = parse_api_error(&api_err, jr)))
        {
            rc = eapi(api_err);
        }
    }
    else
    {
        rc = ehttp(http_status_string(http));
    }

cleanup:
    return rc;
}

enum rc api_scan_seq_count(int64_t scan_id, unsigned *count)
{
    reset_api_error();
    struct count c = {0};

    enum rc rc = RC_OK;

    long http = 0;
    if ((rc = get(query("/scans/%" PRId64 "/seqs/count", scan_id), &http)))
        goto cleanup;

    if ((rc = parse_json())) goto cleanup;

    if (http == 200)
    {
        rc = count_parse(&c, jr);
        *count = c.count;
    }
    else if (http == 404)
    {
        if (!(rc = parse_api_error(&api_err, jr)))
        {
            if (api_err.rc == 5)
                rc = RC_END;
            else
                rc = eapi(api_err);
        }
    }
    else if (recognized_http_status(http))
    {
        if (!(rc = parse_api_error(&api_err, jr)))
        {
            rc = eapi(api_err);
        }
    }
    else
    {
        rc = ehttp(http_status_string(http));
    }

cleanup:
    return rc;
}

enum rc api_scan_submit(int64_t db_id, bool multi_hits, bool hmmer3_compat,
                        char const *filepath, struct sched_job *job)
{
    reset_api_error();

    xfile_basename(filename, filepath, sizeof filename);
    MIME_FILE_DEF(mime, "fasta_file", filename, "text/plain");

    long http = 0;

    // body_reset(response);
    enum rc rc = xcurl_upload2("/scans/", &http, db_id, multi_hits,
                               hmmer3_compat, &mime, filepath);
    if (rc) goto cleanup;

    if ((rc = parse_json())) goto cleanup;

    if (http == 201)
    {
        sched_job_init(job);
        rc = sched_job_parse(job, jr);
    }
    else if (recognized_http_status(http))
    {
        if (!(rc = parse_api_error(&api_err, jr)))
        {
            rc = eapi(api_err);
        }
    }
    else
    {
        rc = ehttp(http_status_string(http));
    }

cleanup:
    return rc;
}

enum rc api_scan_next_seq(int64_t scan_id, int64_t seq_id,
                          struct sched_seq *seq)
{
    sched_seq_init(seq);
    reset_api_error();

    enum rc rc = RC_OK;

    char const *q =
        query("/scans/%" PRId64 "/seqs/next/%" PRId64, scan_id, seq_id);

    long http = 0;
    if ((rc = get(q, &http))) goto cleanup;

    if ((rc = parse_json())) goto cleanup;

    if (http == 200)
    {
        rc = sched_seq_parse(seq, jr);
    }
    else if (http == 404)
    {
        if (!(rc = parse_api_error(&api_err, jr)))
        {
            if (api_err.rc == 7)
                rc = RC_END;
            else
                rc = eapi(api_err);
        }
    }
    else if (recognized_http_status(http))
    {
        if (!(rc = parse_api_error(&api_err, jr)))
        {
            rc = eapi(api_err);
        }
    }
    else
    {
        rc = ehttp(http_status_string(http));
    }

cleanup:
    return rc;
}

enum rc api_scan_get_by_job_id(int64_t job_id, struct sched_scan *scan)
{
    sched_scan_init(scan);
    reset_api_error();

    long http = 0;
    enum rc rc = get(query("/jobs/%" PRId64 "/scan", job_id), &http);
    if (rc) goto cleanup;

    if ((rc = parse_json())) goto cleanup;

    if (http == 200)
    {
        rc = sched_scan_parse(scan, jr);
    }
    else if (recognized_http_status(http))
    {
        if (!(rc = parse_api_error(&api_err, jr)))
        {
            rc = eapi(api_err);
        }
    }
    else
    {
        rc = ehttp(http_status_string(http));
    }

cleanup:
    return rc;
}

enum rc api_prods_file_up(char const *filepath)
{
    reset_api_error();

    MIME_FILE_DEF(mime, "prods_file", "prods_file.tsv",
                  "text/tab-separated-values");

    long http = 0;
    enum rc rc = upload("/prods/", &http, &mime, filepath);
    if (rc) goto cleanup;

    if ((rc = parse_json())) goto cleanup;

    if (http == 201)
    {
        if (!(jr_type(jr) == JR_ARRAY && jr_nchild(jr) == 0))
            rc = einval("expected empty array");
    }
    else if (recognized_http_status(http))
    {
        if (!(rc = parse_api_error(&api_err, jr)))
        {
            rc = eapi(api_err);
        }
    }
    else
    {
        rc = ehttp(http_status_string(http));
    }

cleanup:
    return rc;
}

static enum rc parse_api_error(struct api_error *e, struct jr jr[])
{
    e->rc = jr_long_of(jr, "rc");
    jr_strcpy_of(jr, "msg", e->msg, sizeof e->msg);

    return jr_error() ? einval("failed to parse api_error") : RC_OK;
}
