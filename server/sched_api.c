#include "deciphon/server/sched_api.h"
#include "deciphon/buff.h"
#include "deciphon/logger.h"
#include "deciphon/model/profile_typeid.h"
#include "deciphon/nanoprintf.h"
#include "deciphon/server/sched.h"
#include "deciphon/server/xcurl.h"
#include "deciphon/spinlock.h"
#include "deciphon/to.h"
#include "deciphon/xmath.h"
#include "xjson.h"
#include <inttypes.h>
#include <string.h>

static spinlock_t lock = SPINLOCK_INIT;
static unsigned initialized = 0;
static struct buff *request_body = 0;
static struct buff *response_body = 0;
static struct xjson xjson = {0};

static enum rc parse_error(struct sched_api_error *rerr, struct xjson *x,
                           unsigned start);

static enum rc body_add(struct buff **body, size_t size, char const *data)
{
    if (size == 0) return RC_OK;

    if (!buff_ensure(body, (*body)->size + size))
        return enomem("buff_ensure failed");

    memcpy((*body)->data + (*body)->size, data, size);
    (*body)->size += size;
    return RC_OK;
}

static inline enum rc body_add_str(struct buff **body, char const *str)
{
    return body_add(body, strlen(str), str);
}

static inline size_t body_store(void *data, size_t size, void *arg)
{
    return body_add(arg, size, data) ? 0 : size;
}

static inline void body_reset(struct buff *body) { body->size = 0; }

static inline enum rc body_finish_up(struct buff **body)
{
    return body_add(body, 1, "\0");
}

static inline enum rc parse_json(void)
{
    return xjson_parse(&xjson, response_body->data, response_body->size);
}

enum rc sched_api_init(char const *url_stem)
{
    if (initialized++) return RC_OK;

    enum rc rc = xcurl_init(url_stem);
    if (rc) return rc;

    if (!(request_body = buff_new(1024)))
    {
        xcurl_cleanup();
        return enomem("buff_new failed");
    }

    if (!(response_body = buff_new(1024)))
    {
        buff_del(request_body);
        xcurl_cleanup();
        return enomem("buff_new failed");
    }
    return RC_OK;
}

void sched_api_cleanup(void)
{
    if (!initialized) return;
    if (--initialized) return;

    xcurl_cleanup();
    buff_del(request_body);
    buff_del(response_body);
}

enum rc sched_api_wipe(void)
{
    spinlock_lock(&lock);

    long http_code = 0;
    enum rc rc = xcurl_delete("/", &http_code);

    spinlock_unlock(&lock);
    return rc;
}

enum rc sched_api_add_db(struct sched_db *db, struct sched_api_error *rerr)
{
    spinlock_lock(&lock);
    sched_api_error_init(rerr);

    enum rc rc = RC_OK;

    body_reset(request_body);
    if ((rc = body_add_str(&request_body, "{\"filename\": \""))) goto cleanup;
    if ((rc = body_add_str(&request_body, db->filename))) goto cleanup;
    if ((rc = body_add_str(&request_body, "\"}"))) goto cleanup;
    rc = body_finish_up(&request_body);
    if (rc) goto cleanup;

    long http_code = 0;
    body_reset(response_body);
    rc = xcurl_post("/dbs/", &http_code, body_store, &response_body,
                    request_body->data);
    if (rc) goto cleanup;
    rc = body_finish_up(&response_body);
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

enum rc sched_api_get_db(struct sched_db *db, struct sched_api_error *rerr)
{
    spinlock_lock(&lock);
    sched_api_error_init(rerr);

    char query[] = "/dbs/00000000000000000000";
    npf_snprintf(query, sizeof(query), "/dbs/%" PRId64, db->id);

    long http_code = 0;
    body_reset(response_body);
    enum rc rc = xcurl_get(query, &http_code, body_store, &response_body);
    if (rc) goto cleanup;
    rc = body_finish_up(&response_body);
    if (rc) goto cleanup;

    if (http_code == 200)
    {
        if ((rc = parse_json())) goto cleanup;
        rc = parse_db_list(db, &xjson);
    }
    else if (http_code == 404 || http_code == 500)
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

enum rc sched_api_post_testing_data(struct sched_api_error *rerr)
{
    spinlock_lock(&lock);
    sched_api_error_init(rerr);

    enum rc rc = RC_OK;

    body_reset(request_body);
    if ((rc = body_add_str(&request_body, "{}"))) goto cleanup;
    rc = body_finish_up(&request_body);
    if (rc) goto cleanup;

    long http_code = 0;
    body_reset(response_body);
    rc = xcurl_post("/testing/data/", &http_code, body_store, &response_body,
                    request_body->data);
    if (rc) goto cleanup;
    rc = body_finish_up(&response_body);
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
    sched_api_error_init(rerr);

    enum rc rc = RC_OK;

    long http_code = 0;
    body_reset(response_body);
    rc = xcurl_get("/jobs/next_pend", &http_code, body_store, &response_body);
    if (rc) goto cleanup;
    rc = body_finish_up(&response_body);
    if (rc) goto cleanup;

    if (http_code == 200)
    {
        if ((rc = parse_json())) goto cleanup;

        if (!xjson_is_array(&xjson, 0))
            rc = einval("expected array");
        else if (xjson_is_array_empty(&xjson, 0))
            rc = RC_END;
        else
            rc = sched_job_parse(job, &xjson, 2);
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

enum rc sched_api_scan_next_seq(int64_t scan_id, struct sched_seq *seq,
                                struct sched_api_error *rerr)
{
    spinlock_lock(&lock);
    sched_api_error_init(rerr);

    char query[] = "/scans/00000000000000000000/seqs/next/00000000000000000000";
    npf_snprintf(query, sizeof(query), "/jobs/%" PRId64 "/seqs/next/%" PRId64,
                 scan_id, seq->id);

    long http_code = 0;
    body_reset(response_body);
    enum rc rc = xcurl_get(query, &http_code, body_store, &response_body);
    if (rc) goto cleanup;
    rc = body_finish_up(&response_body);
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
    enum rc rc = RC_OK;

    char buf[] = "/jobs/00000000000000000000";

    body_reset(request_body);
    if ((rc = body_add_str(&request_body, "{\"job_id\": "))) return rc;
    npf_snprintf(buf, sizeof(buf), "%" PRId64, job_id);

    if ((rc = body_add_str(&request_body, buf))) return rc;
    if ((rc = body_add_str(&request_body, ", \"state\": \""))) return rc;

    if ((rc = body_add_str(&request_body, job_states[state]))) return rc;

    if ((rc = body_add_str(&request_body, "\", \"error\": \""))) return rc;

    if ((rc = body_add_str(&request_body, state_error))) return rc;

    if ((rc = body_add_str(&request_body, "\"}"))) return rc;

    rc = body_finish_up(&request_body);
    if (rc) return rc;

    npf_snprintf(buf, sizeof(buf), "/jobs/%" PRId64, job_id);
    body_reset(response_body);
    rc = xcurl_patch(buf, http_code, body_store, &response_body,
                     request_body->data);
    if (rc) return rc;
    return body_finish_up(&response_body);
}

enum rc sched_api_set_job_state(int64_t job_id, enum sched_job_state state,
                                char const *msg, struct sched_api_error *rerr)
{
    spinlock_lock(&lock);

    static struct sched_job job = {0};
    job.id = job_id;

    long http_code = 0;
    enum rc rc = set_job_state(job_id, state, msg, &http_code);

    if (http_code == 200)
    {
        if ((rc = parse_json())) goto cleanup;
        rc = sched_job_parse(&job, &xjson, 1);
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

enum rc sched_api_download_db(struct sched_db *db, FILE *fp)
{
    spinlock_lock(&lock);

    enum rc rc = RC_OK;

    char query[] = "/dbs/00000000000000000000/download";

    npf_snprintf(query, sizeof(query), "/dbs/%" PRId64 "/download", db->id);
    long http_code = 0;
    rc = xcurl_download(query, &http_code, fp);
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

    enum rc rc = RC_OK;
    struct xcurl_mime mime = {0};
    xcurl_mime_set(&mime, "prods_file", "prods_file.tsv",
                   "text/tab-separated-values");

    long http_code = 0;
    body_reset(response_body);
    rc = xcurl_upload("/prods/", &http_code, body_store, &response_body, &mime,
                      filepath);
    if (rc) goto cleanup;
    rc = body_finish_up(&response_body);
    if (rc) goto cleanup;

    if (http_code == 201)
    {
        if ((rc = parse_json())) goto cleanup;
        if (!(xjson_is_array(&xjson, 0) && xjson_is_array_empty(&xjson, 0)))
            rc = einval("expected empty array");
    }
    else if (http_code == 400 || http_code == 409)
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
