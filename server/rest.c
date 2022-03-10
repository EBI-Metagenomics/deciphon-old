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

static struct xcurl xcurl = {0};
static spinlock_t lock = SPINLOCK_INIT;
static atomic_bool initialized = false;
static struct buff *buff = 0;
struct xjson xjson = {0};

static size_t parse_job(char const *data, size_t size);
static size_t parse_db(char const *data, size_t size);

static size_t save_to_buff(void *data, size_t size, void *arg)
{
    printf("Callback save_to_buff\n");
    fflush(stdout);
    if (size == 0) return 0;
    (void)arg;

    struct buff *tmp = buff_ensure(buff, buff->size + size);
    if (!tmp)
    {
        enomem("buff_ensure failed");
        return 0;
    }
    memcpy(buff->data + buff->size, data, size);
    return size;
}

static void reset_buff(void) { buff->size = 0; }

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
    spinlock_lock(&lock);

    enum rc rc = RC_OK;
    if (initialized)
    {
        rc = einval("cannot have multiple rest open");
        goto cleanup;
    }

    rc = xcurl_init(&xcurl, url_stem);
    buff = buff_new(1024);
    if (!buff) rc = enomem("buff_new failed");

    initialized = true;

cleanup:
    spinlock_unlock(&lock);
    return rc;
}

void rest_close(void)
{
    spinlock_lock(&lock);
    xcurl_del(&xcurl);
    initialized = false;
    buff_del(buff);
    spinlock_unlock(&lock);
}

enum rc rest_wipe(void)
{
    spinlock_lock(&lock);

    reset_buff();
    long http_code = 0;
    enum rc rc = xcurl_delete(&xcurl, "/", &http_code);

    spinlock_unlock(&lock);
    return rc;
}

enum rc rest_next_pend_job(struct sched_job *job)
{
    spinlock_lock(&lock);

    reset_buff();
    long http_code = 0;
    enum rc rc =
        xcurl_get(&xcurl, "/jobs/next_pend", &http_code, save_to_buff, 0);

    spinlock_unlock(&lock);
    return rc;
}

enum rc rest_post_db(struct sched_db *db)
{
    spinlock_lock(&lock);

    char body[FILENAME_SIZE + sizeof("{\"filename\": \"\"}")] = {0};
    npf_snprintf(body, sizeof(body), "{\"filename\": \"%s\"}", db->filename);

    reset_buff();
    long http_code = 0;
    enum rc rc = xcurl_post(&xcurl, "/dbs/", &http_code, save_to_buff, 0, body);

    spinlock_unlock(&lock);
    return rc;
}

enum rc rest_get_db(struct sched_db *db)
{
    spinlock_lock(&lock);

    char query[] = "/dbs/18446744073709551615";
    npf_snprintf(query, sizeof(query), "/dbs/%" PRId64, db->id);

    reset_buff();
    long http_code = 0;
    enum rc rc = xcurl_get(&xcurl, query, &http_code, save_to_buff, 0);

    spinlock_unlock(&lock);
    return rc;
}

static size_t parse_job(char const *data, size_t size)
{
    enum rc rc = xjson_parse(&xjson, data, size);
    if (rc) return rc;

    return RC_OK;
}

static size_t parse_db(char const *data, size_t size)
{
    enum rc rc = xjson_parse(&xjson, data, size);
    if (rc) return rc;

    return RC_OK;
}
