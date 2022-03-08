#include "deciphon/server/rest.h"
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
    enum rc rc = xcurl_init(&xcurl, url_stem);
    spinlock_unlock(&lock);
    return rc;
}

void rest_close(void)
{
    spinlock_lock(&lock);
    xcurl_del(&xcurl);
    spinlock_unlock(&lock);
}

static size_t parse_wipe(void *data, size_t size, void *arg)
{
    (void)data;
    (void)arg;
    return size;
}

enum rc rest_wipe(void)
{
    long http_code = 0;
    return xcurl_delete(&xcurl, "/", &http_code, parse_wipe, 0);
}

static size_t parse_job(void *data, size_t size, void *arg) { return size; }

enum rc rest_next_pend_job(struct sched_job *job)
{
    long http_code = 0;
    enum rc rc =
        xcurl_get(&xcurl, "/jobs/next_pend", &http_code, parse_job, job);
    return rc;
}

static size_t parse_db(void *data, size_t size, void *arg) { return size; }

#if 0
struct data_file
{
    FILE *fp;
    char *data;
};

enum rc data_file_new(void)
{
    enum rc rc = RC_OK;
    struct data_file *f = malloc(sizeof(*f));
    FILE *fp = tmpfile();
    if (!fp)
    {
        rc = eio("create tmpfile");
        goto cleanup;
    }
}

enum rc data_file_del(struct data_file *f)
{
    fclose(f->fp);
    free(f);
}
#endif

enum rc rest_post_db(struct sched_db *db)
{
    char body[FILENAME_SIZE + sizeof("{\"filename\": \"\"}")] = {0};
    npf_snprintf(body, sizeof(body), "{\"filename\": \"%s\"}", db->filename);

    long http_code = 0;
    enum rc rc = xcurl_post(&xcurl, "/dbs/", &http_code, parse_db, db, body);

    return rc;
}

enum rc rest_get_db(struct sched_db *db)
{
    char query[] = "/dbs/18446744073709551615";
    npf_snprintf(query, sizeof(query), "/dbs/%" PRId64, db->id);

    long http_code = 0;
    enum rc rc = xcurl_get(&xcurl, query, &http_code, parse_db, db);

    return rc;
}
