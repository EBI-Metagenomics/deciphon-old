#include "deciphon/server/rest.h"
#include "deciphon/buff.h"
#include "deciphon/logger.h"
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

static size_t parse_job(char const *data, size_t size);
static size_t parse_db(struct sched_db *db, size_t size, char const *data);

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

enum rc rest_next_pend_job(struct sched_job *job)
{
    spinlock_lock(&rest.lock);

    body_reset(rest.request_body);
    long http_code = 0;
    enum rc rc = xcurl_get(&rest.xcurl, "/jobs/next_pend", &http_code,
                           body_store, rest.response_body);

    spinlock_unlock(&rest.lock);
    return rc;
}

enum rc rest_post_db(struct sched_db *db)
{
    spinlock_lock(&rest.lock);
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
        parse_db(db, rest.response_body->size, rest.response_body->data);
    }
    // 201 OK
    // 409 conflict
    // 500 server error

cleanup:
    spinlock_unlock(&rest.lock);
    return rc;
}

enum rc rest_get_db(struct sched_db *db)
{
    spinlock_lock(&rest.lock);

    char query[] = "/dbs/18446744073709551615";
    npf_snprintf(query, sizeof(query), "/dbs/%" PRId64, db->id);

    body_reset(rest.request_body);
    body_reset(rest.response_body);
    long http_code = 0;
    enum rc rc = xcurl_get(&rest.xcurl, query, &http_code, body_store,
                           &rest.response_body);

    spinlock_unlock(&rest.lock);
    return rc;
}

static size_t parse_job(char const *data, size_t size)
{
    enum rc rc = xjson_parse(&rest.xjson, data, size);
    if (rc) return rc;

    return RC_OK;
}

static size_t parse_db(struct sched_db *db, size_t size, char const *data)
{
    struct xjson *x = &rest.xjson;
    enum rc rc = xjson_parse(x, data, size);
    if (rc) return rc;

    if (x->ntoks != 2 * 3 + 1) return einval("expected three items");

    for (unsigned i = 1; i < x->ntoks; i += 2)
    {
        if (xjson_eqstr(x, i, "id"))
        {
            if ((rc = xjson_bind_int64(x, i + 1, &db->id))) return rc;
        }
        else if (xjson_eqstr(x, i, "xxh3_64"))
        {
            if ((rc = xjson_bind_int64(x, i + 1, &db->xxh3_64))) return rc;
        }
        else if (xjson_eqstr(x, i, "filename"))
        {
            if ((rc = xjson_copy_str(x, i + 1, PATH_SIZE, db->filename)))
                return rc;
        }
        else
            return einval("unexpected json key");
    }
    return RC_OK;
}
