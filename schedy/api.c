#include "api.h"
#include "core/logy.h"
#include "core/sched.h"
#include "http.h"
#include "jx.h"
#include "mime.h"
#include "xcurl.h"
#include "zc.h"
#include <inttypes.h>
#include <stdarg.h>
#include <string.h>

static struct api_error api_err = {0};
static JR_DECLARE(jr, 128);

static enum rc api_error_parse(void);
static void api_error_reset(void);
static enum rc download(char const *query, FILE *fp);
static enum rc get(char const *query);
static enum rc handle_http_exception(void);
static bool is_empty_json_object(void);
static char const *inc_progress_json(int increment);
static char const *job_state_json(int64_t, enum sched_job_state, char const *);
static bool known_http_code(long http_code);
static enum rc parse_json_body(void);
static enum rc patch(char const *query, char const *request);
static char const *query(char const *fmt, ...);

enum rc api_init(char const *url_stem, char const *api_key)
{
    JR_INIT(jr);
    return xcurl_init(url_stem, api_key);
}

void api_cleanup(void) { xcurl_cleanup(); }

struct api_error const *api_error(void) { return &api_err; }

bool api_is_reachable(void)
{
    api_error_reset();
    if (xcurl_get("/")) return false;
    return xcurl_http_code() == 200;
}

enum rc api_wipe(void)
{
    api_error_reset();
    enum rc rc = xcurl_delete("/sched/wipe");
    if (rc) return rc;
    return xcurl_http_code() != 200 ? handle_http_exception() : RC_OK;
}

enum rc api_hmm_up(char const *filepath, struct sched_hmm *hmm)
{
    api_error_reset();

    xcurl_mime_init();
    xcurl_mime_addfile("hmm_file", zc_basename(filepath), MIME_PLAIN, filepath);

    enum rc rc = xcurl_upload("/hmms/");
    if (rc) goto cleanup;

    if ((rc = parse_json_body())) goto cleanup;

    if (xcurl_http_code() == 201)
        rc = sched_hmm_parse(hmm, jr);
    else
        rc = handle_http_exception();

cleanup:
    xcurl_mime_cleanup();
    return rc;
}

enum rc api_hmm_dl(int64_t id, FILE *fp)
{
    api_error_reset();

    enum rc rc = download(query("/hmms/%lld/download", id), fp);
    if (rc) return rc;

    return xcurl_http_code() != 200 ? handle_http_exception() : RC_OK;
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
    api_error_reset();

    enum rc rc = RC_OK;
    if (type == HMM_ID) rc = get(query("/hmms/%lld", p.i));
    if (type == XXH3) rc = get(query("/hmms/xxh3/%lld", p.i));
    if (type == JOB_ID) rc = get(query("/jobs/job_id/%lld", p.i));
    if (type == FILENAME) rc = get(query("/hmms/filename/%s", p.s));
    if (rc) return rc;

    if ((rc = parse_json_body())) return rc;

    if (xcurl_http_code() == 200) return sched_hmm_parse(hmm, jr);

    return handle_http_exception();
}

enum rc api_db_up(char const *filepath, struct sched_db *db)
{
    api_error_reset();

    xcurl_mime_init();
    xcurl_mime_addfile("db_file", zc_basename(filepath), MIME_OCTET, filepath);
    enum rc rc = xcurl_upload("/dbs/");
    if (rc) goto cleanup;

    if ((rc = parse_json_body())) goto cleanup;

    if (xcurl_http_code() == 201)
        rc = sched_db_parse(db, jr);
    else
        rc = handle_http_exception();

cleanup:
    xcurl_mime_cleanup();
    return rc;
}

enum rc api_db_dl(int64_t id, FILE *fp)
{
    enum rc rc = download(query("/dbs/%lld/download", id), fp);
    if (rc) return rc;

    return xcurl_http_code() == 200 ? RC_OK : handle_http_exception();
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
    api_error_reset();

    enum rc rc = RC_OK;
    if (type == DB_ID) rc = get(query("/dbs/%lld", p.i));
    if (type == XXH3) rc = get(query("/dbs/xxh3/%lld", p.i));
    if (type == JOB_ID) rc = get(query("/jobs/%lld/db", p.i));
    if (type == HMM_ID) rc = get(query("/jobs/hmm_id/%lld/db", p.i));
    if (type == FILENAME) rc = get(query("/dbs/filename/%s", p.s));
    if (rc) return rc;

    if ((rc = parse_json_body())) return rc;

    if (xcurl_http_code() == 200) return sched_db_parse(db, jr);

    return handle_http_exception();
}

enum rc api_job_get_by_id(int64_t job_id, struct sched_job *job)
{
    api_error_reset();

    enum rc rc = get(query("/jobs/%lld", job_id));
    if (rc) return rc;

    if ((rc = parse_json_body())) return rc;

    if (xcurl_http_code() == 200) return sched_job_parse(job, jr);

    if (xcurl_http_code() == 204 && is_empty_json_object()) return RC_END;

    return handle_http_exception();
}

enum rc api_job_inc_progress(int64_t job_id, int increment)
{
    api_error_reset();

    enum rc rc = patch(query("/jobs/%lld/progress", job_id),
                       inc_progress_json(increment));
    if (rc) return rc;

    if ((rc = parse_json_body())) return rc;

    if (xcurl_http_code() == 200) return RC_OK;

    return handle_http_exception();
}

enum rc api_job_next_pend(struct sched_job *job)
{
    api_error_reset();

    enum rc rc = get("/jobs/next_pend");
    if (rc) return rc;

    if ((rc = parse_json_body())) return rc;

    if (xcurl_http_code() == 200) return sched_job_parse(job, jr);

    if (xcurl_http_code() == 204 && is_empty_json_object()) return RC_END;

    return handle_http_exception();
}

enum rc api_job_set_state(int64_t job_id, enum sched_job_state state,
                          char const *msg)
{
    api_error_reset();

    enum rc rc = patch(query("/jobs/%lld/state", job_id),
                       job_state_json(job_id, state, msg));
    if (rc) return rc;

    if ((rc = parse_json_body())) return rc;

    if (xcurl_http_code() == 200) return RC_OK;

    return handle_http_exception();
}

enum rc api_scan_seq_count(int64_t scan_id, unsigned *count)
{
    api_error_reset();

    enum rc rc = get(query("/scans/%lld/seqs/count", scan_id));
    if (rc) return rc;

    if ((rc = parse_json_body())) return rc;

    if (xcurl_http_code() == 200)
    {
        *count = (unsigned)jr_ulong_of(jr, "count");
        if (jr_error()) rc = einval("failed to parse count");
        return rc;
    }

    return handle_http_exception();
}

enum rc api_scan_submit(int64_t db_id, bool multi_hits, bool hmmer3_compat,
                        char const *filepath, struct sched_job *job)
{
    api_error_reset();

    xcurl_mime_init();
    static char db_id_str[18] = {0};
    sprintf(db_id_str, "%lld", db_id);
    xcurl_mime_addpart("db_id", db_id_str);
    xcurl_mime_addpart("multi_hits", multi_hits ? "true" : "false");
    xcurl_mime_addpart("hmmer3_compat", hmmer3_compat ? "true" : "false");
    xcurl_mime_addfile("fasta_file", zc_basename(filepath), MIME_PLAIN,
                       filepath);
    enum rc rc = xcurl_upload("/scans/");
    if (rc) goto cleanup;

    if ((rc = parse_json_body())) goto cleanup;

    if (xcurl_http_code() == 201)
    {
        rc = sched_job_parse(job, jr);
    }
    else
        rc = handle_http_exception();

cleanup:
    xcurl_mime_cleanup();
    return rc;
}

enum rc api_scan_next_seq(int64_t scan_id, int64_t seq_id,
                          struct sched_seq *seq)
{
    api_error_reset();

    enum rc rc = get(query("/scans/%lld/seqs/next/%lld", scan_id, seq_id));
    if (rc) return rc;

    if ((rc = parse_json_body())) return rc;

    if (xcurl_http_code() == 200) return sched_seq_parse(seq, jr);

    if (xcurl_http_code() == 204 && is_empty_json_object()) return RC_END;

    return handle_http_exception();
}

enum rc api_scan_dl_seqs(int64_t id, FILE *fp)
{
    api_error_reset();

    enum rc rc = download(query("/scans/%lld/seqs/download", id), fp);
    if (rc) return rc;

    return xcurl_http_code() != 200 ? handle_http_exception() : RC_OK;
}

enum rc api_scan_get_by_id(int64_t id, struct sched_scan *scan)
{
    api_error_reset();

    enum rc rc = get(query("/scans/%lld", id));
    if (rc) return rc;

    if ((rc = parse_json_body())) return rc;

    if (xcurl_http_code() == 200) return sched_scan_parse(scan, jr);

    return handle_http_exception();
}

enum rc api_scan_get_by_job_id(int64_t job_id, struct sched_scan *scan)
{
    api_error_reset();

    enum rc rc = get(query("/jobs/%lld/scan", job_id));
    if (rc) return rc;

    if ((rc = parse_json_body())) return rc;

    if (xcurl_http_code() == 200) return sched_scan_parse(scan, jr);

    return handle_http_exception();
}

enum rc api_prods_file_up(char const *filepath)
{
    api_error_reset();

    xcurl_mime_init();
    xcurl_mime_addfile("prods_file", "prods_file.tsv", MIME_TAB, filepath);
    enum rc rc = xcurl_upload("/prods/");
    if (rc) goto cleanup;

    if ((rc = parse_json_body())) goto cleanup;

    if (xcurl_http_code() == 201 && is_empty_json_object()) goto cleanup;

    rc = handle_http_exception();

cleanup:
    xcurl_mime_cleanup();
    return rc;
}

static enum rc api_error_parse(void)
{
    struct api_error *e = &api_err;
    e->rc = jr_long_of(jr, "rc");
    jr_strcpy_of(jr, "msg", e->msg, sizeof e->msg);

    return jr_error() ? einval("failed to parse api_error") : RC_OK;
}

static void api_error_reset(void)
{
    api_err.rc = 0;
    api_err.msg[0] = 0;
}

static enum rc download(char const *query, FILE *fp)
{
    return xcurl_download(query, fp);
}

static enum rc get(char const *query) { return xcurl_get(query); }

#define eapi(x)                                                                \
    ({                                                                         \
        error(" api_rc[%d] %s", (x).rc, (x).msg);                              \
        RC_EAPI;                                                               \
    })

static enum rc handle_http_exception(void)
{
    enum rc rc = RC_OK;
    if (known_http_code(xcurl_http_code()))
    {
        if (!(rc = api_error_parse())) rc = eapi(api_err);
    }
    else
        rc = ehttp("%s", http_strcode(xcurl_http_code()));
    return rc;
}

static bool is_empty_json_object(void)
{
    return jr_type(jr) == JR_OBJECT && jr_nchild(jr) == 0;
}

static char const *inc_progress_json(int increment)
{
    static char request[128] = {0};

    char *p = request;
    p += jw_object_open(p);
    p += jw_string(p, "increment");
    p += jw_colon(p);
    p += jw_long(p, increment);
    p += jw_object_close(p);
    *p = 0;

    return request;
}

static char const *job_state_json(int64_t job_id, enum sched_job_state state,
                                  char const *error)
{
    static char const job_states[][5] = {[SCHED_PEND] = "pend",
                                         [SCHED_RUN] = "run",
                                         [SCHED_DONE] = "done",
                                         [SCHED_FAIL] = "fail"};
    static char request[512] = {0};

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
    p += jw_string(p, error);

    p += jw_object_close(p);
    *p = 0;

    return request;
}

static bool known_http_code(long http_code)
{
    return http_code == 401 || http_code == 404 || http_code == 406 ||
           http_code == 418 || http_code == 422;
}

static enum rc parse_json_body(void)
{
    return jr_parse(jr, xcurl_body_size(), xcurl_body_data()) ? RC_EFAIL
                                                              : RC_OK;
}

static enum rc patch(char const *query, char const *request)
{
    return xcurl_patch(query, request);
}

static char const *query(char const *fmt, ...)
{
    static char data[128] = {0};
    va_list args;
    va_start(args, fmt);
    vsnprintf(data, sizeof data, fmt, args);
    va_end(args);
    return data;
}
