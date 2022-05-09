#include "deciphon/sched/api.h"
#include "deciphon/core/buff.h"
#include "deciphon/core/http.h"
#include "deciphon/core/ljson.h"
#include "deciphon/core/logging.h"
#include "deciphon/core/to.h"
#include "deciphon/core/xfile.h"
#include "deciphon/core/xmath.h"
#include "deciphon/sched/sched.h"
#include "deciphon/sched/xcurl.h"
#include "xjson.h"
#include <inttypes.h>
#include <omp.h>
#include <stdarg.h>
#include <string.h>

omp_lock_t lock = {0};
static unsigned initialized = 0;
static struct buff *response = 0;
static struct xjson xjson = {0};
static char filename[SCHED_FILENAME_SIZE] = {0};

static char _query[128] = {0};

static char request[1024] = {0};
static struct ljson_ctx ljson = {0};

static inline void init_api_rc(struct api_rc *err)
{
    err->rc = 0;
    err->msg[0] = 0;
}

static enum rc parse_api_rc(struct api_rc *api_rc, struct xjson *x,
                            unsigned start);

static enum rc body_add(struct buff **body, size_t size, char const *data)
{
    if (!buff_ensure(body, (*body)->size + size))
        return enomem("buff_ensure failed");

    memcpy((*body)->data + (*body)->size - 1, data, size);
    (*body)->size += size;

    (*body)->data[(*body)->size - 1] = '\0';

    return RC_OK;
}

static inline size_t body_store(void *data, size_t size, void *arg)
{
    return body_add(arg, size, data) ? 0 : size;
}

static inline void body_reset(struct buff *body)
{
    body->data[0] = '\0';
    body->size = 1;
}

static inline enum rc parse_json(void)
{
    return xjson_parse(&xjson, response->data, response->size);
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
    if (initialized++) return RC_OK;

    enum rc rc = xcurl_init(url_stem, api_key);
    if (rc) return rc;

    if (!(response = buff_new(1024)))
    {
        xcurl_cleanup();
        return enomem("buff_new failed");
    }
    body_reset(response);

    omp_init_lock(&lock);
    return RC_OK;
}

void api_cleanup(void)
{
    if (!initialized) return;
    if (--initialized) return;

    xcurl_cleanup();
    buff_del(response);
    omp_destroy_lock(&lock);
}

static inline enum rc get(char const *query, long *http_code)
{
    body_reset(response);
    return xcurl_get(query, http_code, body_store, &response);
}

static inline bool recognized_http_status(long http_status)
{
    return http_status == 401 || http_status == 404 || http_status == 406 ||
           http_status == 418 || http_status == 422;
}

bool api_is_reachable(void)
{
    omp_set_lock(&lock);

    bool reachable = false;
    long http_code = 0;
    if (get("/", &http_code)) goto cleanup;

    reachable = http_code == 200;

cleanup:
    omp_unset_lock(&lock);
    return reachable;
}

enum rc api_wipe(void)
{
    omp_set_lock(&lock);

    long http_code = 0;
    enum rc rc = xcurl_delete("/sched/wipe", &http_code);
    if (rc) goto cleanup;

    if (http_code != 200) rc = ehttp(http_status_string(http_code));

cleanup:
    omp_unset_lock(&lock);
    return rc;
}

static inline enum rc upload(char const *query, long *http_code,
                             struct xcurl_mime const *mime,
                             char const *filepath)
{
    body_reset(response);
    return xcurl_upload(query, http_code, body_store, &response, mime,
                        filepath);
}

enum rc api_upload_hmm(char const *filepath, struct sched_hmm *hmm,
                       struct api_rc *api_rc)
{
    omp_set_lock(&lock);

    sched_hmm_init(hmm);
    init_api_rc(api_rc);

    struct xcurl_mime mime = {0};
    xfile_basename(filename, filepath);
    xcurl_mime_set(&mime, "hmm_file", filename, "application/octet-stream");

    long http_code = 0;
    enum rc rc = upload("/hmms/", &http_code, &mime, filepath);
    if (rc) goto cleanup;

    if ((rc = parse_json())) goto cleanup;

    if (http_code == 201)
    {
        rc = sched_hmm_parse(hmm, &xjson, 1);
    }
    else if (recognized_http_status(http_code))
    {
        if (!(rc = parse_api_rc(api_rc, &xjson, 1)))
        {
            rc = eapi(*api_rc);
        }
    }
    else
    {
        rc = ehttp(http_status_string(http_code));
    }

cleanup:
    omp_unset_lock(&lock);
    return rc;
}

static inline enum rc patch(char const *query, long *http_code)
{
    body_reset(response);
    return xcurl_patch(query, http_code, body_store, &response, request);
}

enum rc api_get_hmm(int64_t id, struct sched_hmm *hmm, struct api_rc *api_rc)
{
    omp_set_lock(&lock);

    sched_hmm_init(hmm);
    init_api_rc(api_rc);

    long http_code = 0;
    enum rc rc = get(query("/hmms/%" PRId64, id), &http_code);
    if (rc) goto cleanup;

    if ((rc = parse_json())) goto cleanup;

    if (http_code == 200)
    {
        rc = sched_hmm_parse(hmm, &xjson, 1);
    }
    else if (recognized_http_status(http_code))
    {
        if (!(rc = parse_api_rc(api_rc, &xjson, 1)))
        {
            rc = eapi(*api_rc);
        }
    }
    else
    {
        rc = ehttp(http_status_string(http_code));
    }

cleanup:
    omp_unset_lock(&lock);
    return rc;
}

enum rc api_get_hmm_by_job_id(int64_t job_id, struct sched_hmm *hmm,
                              struct api_rc *api_rc)
{
    omp_set_lock(&lock);

    sched_hmm_init(hmm);
    init_api_rc(api_rc);

    long http_code = 0;
    enum rc rc = get(query("/jobs/%" PRId64 "/hmm", job_id), &http_code);
    if (rc) goto cleanup;

    if ((rc = parse_json())) goto cleanup;

    if (http_code == 200)
    {
        rc = sched_hmm_parse(hmm, &xjson, 1);
    }
    else if (recognized_http_status(http_code))
    {
        if (!(rc = parse_api_rc(api_rc, &xjson, 1)))
        {
            rc = eapi(*api_rc);
        }
    }
    else
    {
        rc = ehttp(http_status_string(http_code));
    }

cleanup:
    omp_unset_lock(&lock);
    return rc;
}

enum rc api_download_hmm(int64_t id, FILE *fp, struct api_rc *api_rc)
{
    omp_set_lock(&lock);

    init_api_rc(api_rc);

    enum rc rc = RC_OK;

    long http_code = 0;
    rc =
        xcurl_download(query("/hmms/%" PRId64 "/download", id), &http_code, fp);
    if (rc) goto cleanup;

    if (http_code == 200)
    {
        rc = RC_OK;
    }
    else if (recognized_http_status(http_code))
    {
        if (!(rc = parse_api_rc(api_rc, &xjson, 1)))
        {
            rc = eapi(*api_rc);
        }
    }
    else
    {
        rc = ehttp(http_status_string(http_code));
    }

cleanup:
    omp_unset_lock(&lock);
    return rc;
}

enum rc api_upload_db(char const *filepath, struct sched_db *db,
                      struct api_rc *api_rc)
{
    omp_set_lock(&lock);

    init_api_rc(api_rc);
    sched_db_init(db);

    struct xcurl_mime mime = {0};
    xfile_basename(filename, filepath);
    xcurl_mime_set(&mime, "db_file", filename, "application/octet-stream");

    long http_code = 0;
    enum rc rc = upload("/dbs/", &http_code, &mime, filepath);
    if (rc) goto cleanup;

    if ((rc = parse_json())) goto cleanup;

    if (http_code == 201)
    {
        rc = sched_db_parse(db, &xjson, 1);
    }
    else if (recognized_http_status(http_code))
    {
        if (!(rc = parse_api_rc(api_rc, &xjson, 1)))
        {
            rc = eapi(*api_rc);
        }
    }
    else
    {
        rc = ehttp(http_status_string(http_code));
    }

cleanup:
    omp_unset_lock(&lock);
    return rc;
}

enum rc api_get_db(int64_t id, struct sched_db *db, struct api_rc *api_rc)
{
    omp_set_lock(&lock);

    sched_db_init(db);
    init_api_rc(api_rc);

    long http_code = 0;
    enum rc rc = get(query("/dbs/%" PRId64, id), &http_code);
    if (rc) goto cleanup;

    if ((rc = parse_json())) goto cleanup;

    if (http_code == 200)
    {
        rc = sched_db_parse(db, &xjson, 1);
    }
    else if (recognized_http_status(http_code))
    {
        if (!(rc = parse_api_rc(api_rc, &xjson, 1)))
        {
            rc = eapi(*api_rc);
        }
    }
    else
    {
        rc = ehttp(http_status_string(http_code));
    }

cleanup:
    omp_unset_lock(&lock);
    return rc;
}

enum rc api_next_pend_job(struct sched_job *job, struct api_rc *api_rc)
{
    omp_set_lock(&lock);

    sched_job_init(job);
    init_api_rc(api_rc);

    enum rc rc = RC_OK;

    long http_code = 0;
    if ((rc = get("/jobs/next_pend", &http_code))) goto cleanup;

    if ((rc = parse_json())) goto cleanup;

    if (http_code == 200)
    {
        rc = sched_job_parse(job, &xjson, 1);
    }
    else if (http_code == 404)
    {
        if (!(rc = parse_api_rc(api_rc, &xjson, 1)))
        {
            if (api_rc->rc == 5)
                rc = RC_END;
            else
                rc = eapi(*api_rc);
        }
    }
    else if (recognized_http_status(http_code))
    {
        if (!(rc = parse_api_rc(api_rc, &xjson, 1)))
        {
            rc = eapi(*api_rc);
        }
    }
    else
    {
        rc = ehttp(http_status_string(http_code));
    }

cleanup:
    omp_unset_lock(&lock);
    return rc;
}

enum rc api_scan_next_seq(int64_t scan_id, int64_t seq_id,
                          struct sched_seq *seq, struct api_rc *api_rc)
{
    omp_set_lock(&lock);

    sched_seq_init(seq);
    init_api_rc(api_rc);

    enum rc rc = RC_OK;

    char const *q =
        query("/scans/%" PRId64 "/seqs/next/%" PRId64, scan_id, seq_id);

    long http_code = 0;
    if ((rc = get(q, &http_code))) goto cleanup;

    if ((rc = parse_json())) goto cleanup;

    if (http_code == 200)
    {
        rc = sched_seq_parse(seq, &xjson, 1);
    }
    else if (http_code == 404)
    {
        if (!(rc = parse_api_rc(api_rc, &xjson, 1)))
        {
            if (api_rc->rc == 7)
                rc = RC_END;
            else
                rc = eapi(*api_rc);
        }
    }
    else if (recognized_http_status(http_code))
    {
        if (!(rc = parse_api_rc(api_rc, &xjson, 1)))
        {
            rc = eapi(*api_rc);
        }
    }
    else
    {
        rc = ehttp(http_status_string(http_code));
    }

cleanup:
    omp_unset_lock(&lock);
    return rc;
}

enum rc api_scan_num_seqs(int64_t scan_id, unsigned *num_seqs,
                          struct api_rc *api_rc)
{
    // TODO: implement an API call for that
    static struct sched_seq seq = {0};

    *num_seqs = 0;
    seq.id = 0;
    enum rc rc = RC_OK;

    while (!(rc = api_scan_next_seq(scan_id, seq.id, &seq, api_rc)))
        *num_seqs += 1;

    if (rc == RC_END) rc = RC_OK;
    return rc;
}

enum rc api_get_scan_by_job_id(int64_t job_id, struct sched_scan *scan,
                               struct api_rc *api_rc)
{
    omp_set_lock(&lock);

    sched_scan_init(scan);
    init_api_rc(api_rc);

    long http_code = 0;
    enum rc rc = get(query("/jobs/%" PRId64 "/scan", job_id), &http_code);
    if (rc) goto cleanup;

    if ((rc = parse_json())) goto cleanup;

    if (http_code == 200)
    {
        rc = sched_scan_parse(scan, &xjson, 1);
    }
    else if (recognized_http_status(http_code))
    {
        if (!(rc = parse_api_rc(api_rc, &xjson, 1)))
        {
            rc = eapi(*api_rc);
        }
    }
    else
    {
        rc = ehttp(http_status_string(http_code));
    }

cleanup:
    omp_unset_lock(&lock);
    return rc;
}

static char const job_states[][5] = {[SCHED_PEND] = "pend",
                                     [SCHED_RUN] = "run",
                                     [SCHED_DONE] = "done",
                                     [SCHED_FAIL] = "fail"};

enum rc set_job_state(int64_t job_id, enum sched_job_state state,
                      char const *state_error, long *http_code)
{
    ljson_open(&ljson, sizeof request, request);
    ljson_int(&ljson, "job_id", job_id);
    ljson_str(&ljson, "state", job_states[state]);
    ljson_str(&ljson, "error", state_error);
    ljson_close(&ljson);
    if (ljson_error(&ljson)) return efail("failed to write json");

    return patch(query("/jobs/%" PRId64 "/state", job_id), http_code);
}

enum rc api_set_job_state(int64_t job_id, enum sched_job_state state,
                          char const *msg, struct api_rc *api_rc)
{
    omp_set_lock(&lock);

    init_api_rc(api_rc);

    long http_code = 0;
    enum rc rc = set_job_state(job_id, state, msg, &http_code);
    if (rc) goto cleanup;

    if ((rc = parse_json())) goto cleanup;

    if (http_code == 200)
    {
        rc = RC_OK;
    }
    else if (recognized_http_status(http_code))
    {
        if (!(rc = parse_api_rc(api_rc, &xjson, 1)))
        {
            rc = eapi(*api_rc);
        }
    }
    else
    {
        rc = ehttp(http_status_string(http_code));
    }

cleanup:
    omp_unset_lock(&lock);
    return rc;
}

enum rc api_download_db(int64_t id, FILE *fp, struct api_rc *api_rc)
{
    omp_set_lock(&lock);

    enum rc rc = RC_OK;

    long http_code = 0;
    rc = xcurl_download(query("/dbs/%" PRId64 "/download", id), &http_code, fp);
    if (rc) goto cleanup;

    if (http_code == 200)
    {
        rc = RC_OK;
    }
    else if (recognized_http_status(http_code))
    {
        if (!(rc = parse_api_rc(api_rc, &xjson, 1)))
        {
            rc = eapi(*api_rc);
        }
    }
    else
    {
        rc = ehttp(http_status_string(http_code));
    }

cleanup:
    omp_unset_lock(&lock);
    return rc;
}

enum rc api_upload_prods_file(char const *filepath, struct api_rc *api_rc)
{
    omp_set_lock(&lock);

    init_api_rc(api_rc);

    struct xcurl_mime mime = {0};
    xcurl_mime_set(&mime, "prods_file", "prods_file.tsv",
                   "text/tab-separated-values");

    long http_code = 0;
    enum rc rc = upload("/prods/", &http_code, &mime, filepath);
    if (rc) goto cleanup;

    if ((rc = parse_json())) goto cleanup;

    if (http_code == 201)
    {
        if (!(xjson_is_array(&xjson, 0) && xjson_is_array_empty(&xjson, 0)))
            rc = einval("expected empty array");
    }
    else if (recognized_http_status(http_code))
    {
        if (!(rc = parse_api_rc(api_rc, &xjson, 1)))
        {
            rc = eapi(*api_rc);
        }
    }
    else
    {
        rc = ehttp(http_status_string(http_code));
    }

cleanup:
    omp_unset_lock(&lock);
    return rc;
}

enum rc api_increment_job_progress(int64_t job_id, int increment,
                                   struct api_rc *api_rc)
{
    omp_set_lock(&lock);

    init_api_rc(api_rc);

    ljson_open(&ljson, sizeof request, request);
    ljson_int(&ljson, "increment", increment);
    ljson_close(&ljson);
    if (ljson_error(&ljson)) return efail("failed to write json");

    long http_code = 0;
    enum rc rc = patch(query("/jobs/%" PRId64 "/progress", job_id), &http_code);
    if (rc) goto cleanup;

    if ((rc = parse_json())) goto cleanup;

    if (http_code == 200)
    {
        rc = RC_OK;
    }
    else if (recognized_http_status(http_code))
    {
        if (!(rc = parse_api_rc(api_rc, &xjson, 1)))
        {
            rc = eapi(*api_rc);
        }
    }
    else
    {
        rc = ehttp(http_status_string(http_code));
    }

cleanup:
    omp_unset_lock(&lock);
    return rc;
}

static enum rc parse_api_rc(struct api_rc *api_rc, struct xjson *x,
                            unsigned start)
{
    enum rc rc = RC_OK;
    if (!x) return rc;

    unsigned nitems = 0;
    for (unsigned i = start; i < x->ntoks && nitems < 2; i += 2)
    {
        if (xjson_eqstr(x, i, "rc"))
        {
            if (!xjson_is_number(x, i + 1)) return einval("expected number");
            rc = xjson_bind_int(x, i + 1, &api_rc->rc);
            if (rc) return rc;
        }
        else if (xjson_eqstr(x, i, "msg"))
        {
            if ((rc = xjson_copy_str(x, i + 1, SCHED_JOB_ERROR_SIZE,
                                     api_rc->msg)))
                return rc;
        }
        else
            return einval("unexpected json key");
        nitems++;
    }

    if (nitems != 2) return einval("expected two items");

    return RC_OK;
}
