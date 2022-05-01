#include "file.h"
#include "deciphon/logger.h"
#include "deciphon/xfile.h"

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

enum rc file_ensure_local(char const *filename, int64_t xxh3,
                          file_fetch_func_t fetch)
{
    enum rc rc = RC_OK;

    if (xfile_exists(filename))
    {
        if (!(rc = ensure_integrity(filename, xxh3))) return rc;
    }

    if ((rc = fetch(filename, xxh3))) return rc;

    return ensure_integrity(filename, xxh3);
}
