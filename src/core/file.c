#define XXH_INLINE_ALL
#include "core/file.h"
#include "core/compiler.h"
#include "core/logy.h"
#include "fs.h"
#include "xxhash.h"
#include <assert.h>

#define BUFFSIZE (8 * 1024)

static enum rc ensure_integrity(char const *filename, long xxh3);

enum rc file_ensure_local(char const *filename, long xxh3,
                          file_download_fn_t *download_cb, void *data)
{
    enum rc rc = RC_OK;

    if (fs_exists(filename))
    {
        if (!(rc = ensure_integrity(filename, xxh3))) return rc;
    }

    if ((rc = download_cb(filename, data))) return rc;

    return ensure_integrity(filename, xxh3);
}

static enum rc ensure_integrity(char const *filename, long xxh3)
{
    long hash = 0;
    enum rc rc = file_hash(filename, &hash);
    if (rc) return rc;
    return xxh3 == hash ? RC_OK : einval("invalid hash");
}

enum rc file_hash(char const *filepath, long *hash)
{
    FILE *fp = fopen(filepath, "rb");
    if (!fp) return eio("fopen");

    enum rc rc = file_phash(fp, hash);
    fclose(fp);
    return rc;
}

enum rc file_phash(FILE *fp, long *hash)
{
    enum rc rc = RC_EFAIL;
    XXH3_state_t *state = XXH3_createState();
    if (!state)
    {
        rc = efail("failed to create state");
        goto cleanup;
    }
    XXH3_64bits_reset(state);

    size_t n = 0;
    unsigned char buffer[BUFFSIZE] = {0};
    while ((n = fread(buffer, sizeof(*buffer), BUFFSIZE, fp)) > 0)
    {
        if (n < BUFFSIZE && ferror(fp))
        {
            rc = eio("fread");
            goto cleanup;
        }

        XXH3_64bits_update(state, buffer, n);
    }
    if (ferror(fp))
    {
        rc = eio("fread");
        goto cleanup;
    }
    rc = RC_OK;

    union
    {
        int64_t const i;
        uint64_t const u;
    } const h = {.u = XXH3_64bits_digest(state)};
    *hash = h.i;

cleanup:
    XXH3_freeState(state);
    return rc;
}
