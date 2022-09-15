#include "core/file.h"
#include "core/logging.h"
#include "core/xfile.h"

static enum rc ensure_integrity(char const *filename, int64_t xxh3);

enum rc file_ensure_local(char const *filename, int64_t xxh3,
                          file_download_fn_t *download_cb, void *data)
{
    enum rc rc = RC_OK;

    if (xfile_exists(filename))
    {
        if (!(rc = ensure_integrity(filename, xxh3))) return rc;
    }

    if ((rc = download_cb(filename, data))) return rc;

    return ensure_integrity(filename, xxh3);
}

static enum rc ensure_integrity(char const *filename, int64_t xxh3)
{
    FILE *fp = fopen(filename, "rb");
    if (!fp) return eio("fopen");

    int64_t hash = 0;
    enum rc rc = xfile_hash(fp, &hash);
    if (rc)
    {
        fclose(fp);
        return rc;
    }
    fclose(fp);
    return xxh3 == hash ? RC_OK : einval("invalid hash");
}
