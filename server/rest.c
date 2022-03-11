#include "deciphon/server/rest.h"
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

static struct
{
    struct xcurl xcurl;
    spinlock_t lock;
    unsigned int initialized;
    struct buff *request_body;
    struct buff *response_body;
    struct xjson xjson;
} rest = {{0}, SPINLOCK_INIT, false, 0, 0, {0}};

static enum rc parse_error(struct rest_error *error, struct xjson *x,
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

static inline void rest_error_reset(struct rest_error *error)
{
    error->rc = RC_OK;
    error->msg[0] = 0;
}

static inline enum rc parse_json(void)
{
    return xjson_parse(&rest.xjson, rest.response_body->data,
                       rest.response_body->size);
}

#if 0
static size_t next_pend_job_callback(void *contents, size_t bsize, size_t nmemb,
                                     void *userp)
{
    struct sched_job *job = userp;
    (void)userp;
    size_t size = bsize * nmemb;
    char *data = contents;

    jsmn_init(&json.parser);
    int r = jsmn_parse(&json.parser, data, size, json.tok, 128);
    if (r < 0)
    {
        return 0;
    }
    if (r < 1 || json.tok[0].type != JSMN_ARRAY)
    {
        return 0;
    }

    json.ntoks = (unsigned)(r - 1);
    if (parse_job(data, json.ntoks, json.tok + 1, job)) return 0;
    return size;
}
#endif

enum rc rest_open(char const *url_stem)
{
    if (rest.initialized++) return RC_OK;

    enum rc rc = xcurl_init(&rest.xcurl, url_stem);
    if (rc) return rc;

    if (!(rest.request_body = buff_new(1024)))
    {
        xcurl_del(&rest.xcurl);
        return enomem("buff_new failed");
    }

    if (!(rest.response_body = buff_new(1024)))
    {
        buff_del(rest.request_body);
        xcurl_del(&rest.xcurl);
        return enomem("buff_new failed");
    }
    return RC_OK;
}

void rest_close(void)
{
    if (!rest.initialized) return;

    if (--rest.initialized) return;

    xcurl_del(&rest.xcurl);
    buff_del(rest.request_body);
    buff_del(rest.response_body);
}

enum rc rest_wipe(void)
{
    spinlock_lock(&rest.lock);

    body_reset(rest.request_body);
    long http_code = 0;
    enum rc rc = xcurl_delete(&rest.xcurl, "/", &http_code);

    spinlock_unlock(&rest.lock);
    return rc;
}

enum rc rest_post_db(struct sched_db *db, struct rest_error *error)
{
    spinlock_lock(&rest.lock);
    rest_error_reset(error);

    enum rc rc = RC_OK;

    body_reset(rest.request_body);
    if ((rc = body_add_str(&rest.request_body, "{\"filename\": \"")))
        goto cleanup;
    if ((rc = body_add_str(&rest.request_body, db->filename))) goto cleanup;
    if ((rc = body_add_str(&rest.request_body, "\"}"))) goto cleanup;

    body_reset(rest.response_body);
    long http_code = 0;
    rc = xcurl_post(&rest.xcurl, "/dbs/", &http_code, body_store,
                    &rest.response_body, rest.request_body->data);
    if (rc) goto cleanup;

    if (http_code == 201)
    {
        if ((rc = parse_json())) goto cleanup;
        rc = sched_db_parse(db, &rest.xjson, 1);
    }
    else if (http_code == 409 || http_code == 500)
    {
        if ((rc = parse_json())) goto cleanup;
        rc = parse_error(error, &rest.xjson, 1);
    }
    else
    {
        rc = efail("unexpected http code");
    }

cleanup:
    spinlock_unlock(&rest.lock);
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

enum rc rest_get_db(struct sched_db *db, struct rest_error *error)
{
    spinlock_lock(&rest.lock);
    rest_error_reset(error);

    char query[] = "/dbs/00000000000000000000";
    npf_snprintf(query, sizeof(query), "/dbs/%" PRId64, db->id);

    body_reset(rest.response_body);
    long http_code = 0;
    enum rc rc = xcurl_get(&rest.xcurl, query, &http_code, body_store,
                           &rest.response_body);
    if (rc) goto cleanup;

    if (http_code == 200)
    {
        if ((rc = parse_json())) goto cleanup;
        rc = parse_db_list(db, &rest.xjson);
    }
    else if (http_code == 404 || http_code == 500)
    {
        if ((rc = parse_json())) goto cleanup;
        rc = parse_error(error, &rest.xjson, 1);
    }
    else
    {
        rc = efail("unexpected http code");
    }

cleanup:
    spinlock_unlock(&rest.lock);
    return rc;
}

static enum rc parse_job_list(struct sched_job *job, struct xjson *x)
{
    if (!xjson_is_array(x, 0)) return einval("expected array");
    if (xjson_is_array_empty(x, 0))
    {
        sched_job_init(job);
        return RC_OK;
    }
    return sched_job_parse(job, x, 2);
}

enum rc rest_testing_data(struct rest_error *error)
{
    spinlock_lock(&rest.lock);
    rest_error_reset(error);

    enum rc rc = RC_OK;

    body_reset(rest.request_body);
    if ((rc = body_add_str(&rest.request_body, "{}"))) goto cleanup;

    long http_code = 0;
    rc = xcurl_post(&rest.xcurl, "/testing/data/", &http_code, body_store,
                    &rest.response_body, rest.request_body->data);
    if (rc) goto cleanup;

    if (http_code == 201)
    {
        if ((rc = parse_json())) goto cleanup;
        if (!(xjson_is_array(&rest.xjson, 0) &&
              xjson_is_array_empty(&rest.xjson, 0)))
        {
            rc = einval("expected empty array");
            goto cleanup;
        }
    }
    else if (http_code == 404 || http_code == 409 || http_code == 500)
    {
        if ((rc = parse_json())) goto cleanup;
        rc = parse_error(error, &rest.xjson, 1);
    }
    else
    {
        rc = efail("unexpected http code");
    }

cleanup:
    spinlock_unlock(&rest.lock);
    return rc;
}

enum rc rest_next_pend_job(struct sched_job *job, struct rest_error *error)
{
    spinlock_lock(&rest.lock);
    rest_error_reset(error);

    enum rc rc = RC_OK;

    body_reset(rest.request_body);
    long http_code = 0;
    rc = xcurl_get(&rest.xcurl, "/jobs/next_pend", &http_code, body_store,
                   &rest.response_body);
    if (rc) goto cleanup;

    if (http_code == 200)
    {
        if ((rc = parse_json())) goto cleanup;
        rc = parse_job_list(job, &rest.xjson);
    }
    else if (http_code == 409 || http_code == 500)
    {
        if ((rc = parse_json())) goto cleanup;
        rc = parse_error(error, &rest.xjson, 1);
    }
    else
    {
        rc = efail("unexpected http code");
    }

cleanup:
    spinlock_unlock(&rest.lock);
    return rc;
}

static enum rc parse_error(struct rest_error *error, struct xjson *x,
                           unsigned start)
{
    enum rc rc = RC_OK;

    unsigned nitems = 0;
    for (unsigned i = start; i < x->ntoks && nitems < 2; i += 2)
    {
        if (xjson_eqstr(x, i, "rc"))
        {
            if (!xjson_is_string(x, i + 1)) return einval("expected string");
            rc = rc_resolve(json_tok_size(x, i + 1), json_tok_value(x, i + 1),
                            &error->rc);
            if (rc) return rc;
        }
        else if (xjson_eqstr(x, i, "msg"))
        {
            if ((rc = xjson_copy_str(x, i + 1, ERROR_SIZE, error->msg)))
                return rc;
        }
        else
            return einval("unexpected json key");
        nitems++;
    }

    if (nitems != 2) return einval("expected two items");

    return RC_OK;
}
