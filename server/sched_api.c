#include "deciphon/server/sched_api.h"
#include "deciphon/buff.h"
#include "deciphon/ljson.h"
#include "deciphon/logger.h"
#include "deciphon/model/profile_typeid.h"
#include "deciphon/nanoprintf.h"
#include "deciphon/server/sched.h"
#include "deciphon/server/xcurl.h"
#include "deciphon/spinlock.h"
#include "deciphon/to.h"
#include "deciphon/xfile.h"
#include "deciphon/xmath.h"
#include "xjson.h"
#include <inttypes.h>
#include <string.h>

static spinlock_t lock = SPINLOCK_INIT;
static unsigned initialized = 0;
static struct buff *response = 0;
static struct xjson xjson = {0};
static char filename[FILENAME_SIZE] = {0};

static char _query[128] = {0};

static char request[1024] = {0};
static struct ljson_ctx ljson = {0};

static enum rc parse_error(struct sched_api_error *rerr, struct xjson *x,
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
    npf_vsnprintf(_query, sizeof _query, fmt, args);
    va_end(args);

    return _query;
}

enum rc sched_api_init(char const *url_stem)
{
    if (initialized++) return RC_OK;

    enum rc rc = xcurl_init(url_stem);
    if (rc) return rc;

    if (!(response = buff_new(1024)))
    {
        xcurl_cleanup();
        return enomem("buff_new failed");
    }
    body_reset(response);

    return RC_OK;
}

void sched_api_cleanup(void)
{
    if (!initialized) return;
    if (--initialized) return;

    xcurl_cleanup();
    buff_del(response);
}

enum rc sched_api_wipe(void)
{
    spinlock_lock(&lock);

    long http_code = 0;
    enum rc rc = xcurl_delete("/sched/wipe", &http_code);
    if (rc) goto cleanup;

    if (http_code != 200) rc = efail("failed to wipe sheduler");

cleanup:
    spinlock_unlock(&lock);
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

enum rc sched_api_upload_hmm(char const *filepath, struct sched_hmm *hmm,
                             struct sched_api_error *rerr)
{
    spinlock_lock(&lock);

    sched_api_error_init(rerr);
    sched_hmm_init(hmm);

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
    else if (http_code == 400 || http_code == 409)
    {
        rc = parse_error(rerr, &xjson, 1);
    }
    else
    {
        rc = efail("unexpected http code");
    }

cleanup:
    spinlock_unlock(&lock);
    return rc;
}

static inline enum rc get(char const *query, long *http_code)
{
    body_reset(response);
    return xcurl_get(query, http_code, body_store, &response);
}

static inline enum rc patch(char const *query, long *http_code)
{
    body_reset(response);
    return xcurl_patch(query, http_code, body_store, &response, request);
}

enum rc sched_api_get_hmm(int64_t id, struct sched_hmm *hmm,
                          struct sched_api_error *rerr)
{
    spinlock_lock(&lock);

    sched_hmm_init(hmm);
    sched_api_error_init(rerr);

    long http_code = 0;
    enum rc rc = get(query("/hmms/%" PRId64, id), &http_code);
    if (rc) goto cleanup;

    if ((rc = parse_json())) goto cleanup;

    if (http_code == 200)
    {
        rc = sched_hmm_parse(hmm, &xjson, 1);
    }
    else if (http_code == 400 || http_code == 409)
    {
        rc = parse_error(rerr, &xjson, 1);
    }
    else
    {
        rc = efail("unexpected http code");
    }

cleanup:
    spinlock_unlock(&lock);
    return rc;
}

enum rc sched_api_get_hmm_by_job_id(int64_t job_id, struct sched_hmm *hmm,
                                    struct sched_api_error *rerr)
{
    spinlock_lock(&lock);

    sched_hmm_init(hmm);
    sched_api_error_init(rerr);

    long http_code = 0;
    enum rc rc = get(query("/jobs/%" PRId64 "/hmm", job_id), &http_code);
    if (rc) goto cleanup;

    if ((rc = parse_json())) goto cleanup;

    if (http_code == 200)
    {
        rc = sched_hmm_parse(hmm, &xjson, 1);
    }
    else if (http_code == 400 || http_code == 409)
    {
        rc = parse_error(rerr, &xjson, 1);
    }
    else
    {
        rc = efail("unexpected http code");
    }

cleanup:
    spinlock_unlock(&lock);
    return rc;
}

enum rc sched_api_upload_db(char const *filepath, struct sched_db *db,
                            struct sched_api_error *rerr)
{
    spinlock_lock(&lock);

    sched_api_error_init(rerr);
    sched_db_init(db);

    struct xcurl_mime mime = {0};
    xfile_basename(filename, filepath);
    xcurl_mime_set(&mime, "database_file", filename,
                   "application/octet-stream");

    long http_code = 0;
    enum rc rc = upload("/dbs/", &http_code, &mime, filepath);
    if (rc) goto cleanup;

    if ((rc = parse_json())) goto cleanup;

    if (http_code == 201)
    {
        rc = sched_db_parse(db, &xjson, 1);
    }
    else if (http_code == 400 || http_code == 409)
    {
        rc = parse_error(rerr, &xjson, 1);
    }
    else
    {
        rc = efail("unexpected http code");
    }

cleanup:
    spinlock_unlock(&lock);
    return rc;
}

enum rc sched_api_add_db(struct sched_db *db, struct sched_api_error *rerr)
{
    spinlock_lock(&lock);

    sched_api_error_init(rerr);
    sched_db_init(db);

    enum rc rc = RC_OK;

    ljson_open(&ljson, sizeof request, request);
    ljson_str(&ljson, "filename", db->filename);
    ljson_close(&ljson);
    if (ljson_error(&ljson)) return efail("failed to write json");

    long http_code = 0;
    body_reset(response);
    rc = xcurl_post("/dbs/", &http_code, body_store, &response, request);
    if (rc) goto cleanup;

    if (http_code == 201)
    {
        if ((rc = parse_json())) goto cleanup;
        rc = sched_db_parse(db, &xjson, 1);
    }
    else if (http_code == 409 || http_code == 500)
    {
        if ((rc = parse_json())) goto cleanup;
        rc = parse_error(rerr, &xjson, 1);
    }
    else
    {
        rc = efail("unexpected http code");
    }

cleanup:
    spinlock_unlock(&lock);
    return rc;
}

#if 0
static enum rc parse_db_list(struct sched_db *db, struct xjson *x)
{
    if (!xjson_is_array(x, 0)) return einval("expected array");
    if (xjson_is_array_empty(x, 0))
    {
        sched_db_init(db);
        return RC_OK;
    }
    return sched_db_parse(db, x, 2);
}
#endif

enum rc sched_api_get_db(int64_t id, struct sched_db *db,
                         struct sched_api_error *rerr)
{
    spinlock_lock(&lock);

    sched_db_init(db);
    sched_api_error_init(rerr);

    long http_code = 0;
    enum rc rc = get(query("/dbs/%" PRId64, id), &http_code);
    if (rc) goto cleanup;

    if ((rc = parse_json())) goto cleanup;

    if (http_code == 200)
    {
        rc = sched_db_parse(db, &xjson, 1);
    }
    else if (http_code == 400 || http_code == 409)
    {
        rc = parse_error(rerr, &xjson, 1);
    }
    else
    {
        rc = efail("unexpected http code");
    }

cleanup:
    spinlock_unlock(&lock);
    return rc;
}

enum rc sched_api_post_testing_data(struct sched_api_error *rerr)
{
    spinlock_lock(&lock);
    sched_api_error_init(rerr);

    enum rc rc = RC_OK;

    long http_code = 0;
    body_reset(response);
    rc = xcurl_post("/testing/data/", &http_code, body_store, &response, "{}");
    if (rc) goto cleanup;

    if (http_code == 201)
    {
        if ((rc = parse_json())) goto cleanup;
        if (!(xjson_is_array(&xjson, 0) && xjson_is_array_empty(&xjson, 0)))
        {
            rc = einval("expected empty array");
            goto cleanup;
        }
    }
    else if (http_code == 404 || http_code == 409 || http_code == 500)
    {
        if ((rc = parse_json())) goto cleanup;
        rc = parse_error(rerr, &xjson, 1);
    }
    else
    {
        rc = efail("unexpected http code");
    }

cleanup:
    spinlock_unlock(&lock);
    return rc;
}

enum rc sched_api_next_pend_job(struct sched_job *job,
                                struct sched_api_error *rerr)
{
    spinlock_lock(&lock);

    sched_job_init(job);
    sched_api_error_init(rerr);

    enum rc rc = RC_OK;

    long http_code = 0;
    rc = get("/jobs/next_pend", &http_code);
    if (rc) goto cleanup;

    if ((rc = parse_json())) goto cleanup;

    if (http_code == 200)
    {
        if (!xjson_is_array(&xjson, 0))
            rc = einval("expected array");
        else if (xjson_is_array_empty(&xjson, 0))
            rc = RC_END;
        else
            rc = sched_job_parse(job, &xjson, 2);
    }
    else if (http_code == 409 || http_code == 500)
    {
        rc = parse_error(rerr, &xjson, 1);
    }
    else
    {
        rc = efail("unexpected http code");
    }

cleanup:
    spinlock_unlock(&lock);
    return rc;
}

enum rc sched_api_scan_next_seq(int64_t scan_id, struct sched_seq *seq,
                                struct sched_api_error *rerr)
{
    spinlock_lock(&lock);
    sched_api_error_init(rerr);

    long http_code = 0;
    enum rc rc =
        get(query("/jobs/%" PRId64 "/seqs/next/%" PRId64, scan_id, seq->id),
            &http_code);
    if (rc) goto cleanup;

    if (http_code == 200)
    {
        if ((rc = parse_json())) goto cleanup;
        if (!xjson_is_array(&xjson, 0)) return einval("expected array");
        if (xjson_is_array_empty(&xjson, 0))
            rc = RC_END;
        else
            rc = sched_seq_parse(seq, &xjson, 2);
    }
    else if (http_code == 409 || http_code == 500)
    {
        if ((rc = parse_json())) goto cleanup;
        rc = parse_error(rerr, &xjson, 1);
    }
    else
    {
        rc = efail("unexpected http code");
    }

cleanup:
    spinlock_unlock(&lock);
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

    return patch(query("/jobs/%" PRId64, job_id), http_code);
}

enum rc sched_api_set_job_state(int64_t job_id, enum sched_job_state state,
                                char const *msg, struct sched_api_error *rerr)
{
    spinlock_lock(&lock);

    sched_api_error_init(rerr);

    long http_code = 0;
    enum rc rc = set_job_state(job_id, state, msg, &http_code);
    if (rc) goto cleanup;

    if ((rc = parse_json())) goto cleanup;

    if (http_code == 409 || http_code == 500)
    {
        rc = parse_error(rerr, &xjson, 1);
    }
    else if (http_code != 200)
    {
        rc = efail("unexpected http code");
    }

cleanup:
    spinlock_unlock(&lock);
    return rc;
}

enum rc sched_api_download_db(struct sched_db *db, FILE *fp)
{
    spinlock_lock(&lock);

    enum rc rc = RC_OK;

    long http_code = 0;
    rc = xcurl_download(query("/dbs/%" PRId64 "/download", db->id), &http_code,
                        fp);
    if (rc) goto cleanup;

    if (http_code == 200)
    {
        rc = RC_OK;
    }
    else if (http_code == 404)
    {
        rc = einval("database not found");
    }
    else
    {
        rc = efail("unexpected http code");
    }

cleanup:
    spinlock_unlock(&lock);
    return rc;
}

enum rc sched_api_upload_prods_file(char const *filepath,
                                    struct sched_api_error *rerr)
{
    spinlock_lock(&lock);

    sched_api_error_init(rerr);

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
    else if (http_code == 400 || http_code == 409)
    {
        rc = parse_error(rerr, &xjson, 1);
    }
    else
    {
        rc = efail("unexpected http code");
    }

cleanup:
    spinlock_unlock(&lock);
    return rc;
}

static enum rc parse_error(struct sched_api_error *rerr, struct xjson *x,
                           unsigned start)
{
    enum rc rc = RC_OK;
    if (!x) return rc;

    unsigned nitems = 0;
    for (unsigned i = start; i < x->ntoks && nitems < 2; i += 2)
    {
        if (xjson_eqstr(x, i, "rc"))
        {
            if (!xjson_is_string(x, i + 1)) return einval("expected string");
            rc = sched_rc_resolve(json_tok_size(x, i + 1),
                                  json_tok_value(x, i + 1), &rerr->rc);
            if (rc) return rc;
        }
        else if (xjson_eqstr(x, i, "msg"))
        {
            if ((rc = xjson_copy_str(x, i + 1, ERROR_SIZE, rerr->msg)))
                return rc;
        }
        else
            return einval("unexpected json key");
        nitems++;
    }

    if (nitems != 2) return einval("expected two items");

    return RC_OK;
}
