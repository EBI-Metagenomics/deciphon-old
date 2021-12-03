#include "db_handle.h"
#include "db.h"
#include "logger.h"
#include "macros.h"
#include "pro_db.h"
#include "safe.h"
#include "utc.h"
#include "xmath.h"
#include <string.h>

void db_handle_init(struct db_handle *db, int64_t id)
{
    db->id = id;
    db->pool_id = 0;
    cco_hnode_init(&db->hnode);
    db->nfiles = 0;
    memset(db->fp, 0, MEMBER_SIZE(*db, fp));
    db->open_since = 0;
}

static enum rc close_files(struct db_handle *db, int start, int end,
                               bool ignore_error)
{
    enum rc rc = DONE;
    for (int i = start; i < end; ++i)
    {
        if (fclose(db->fp[i]) && !ignore_error)
        {
            rc = error(IOERROR, "failed to close db");
            break;
        }
        db->nfiles--;
    }
    return rc;
}

static enum rc open_files(struct db_handle *db, int start, int end,
                              char path[DCP_PATH_SIZE])
{
    enum rc rc = DONE;
    for (int i = start; i < end; ++i)
    {
        if (!(db->fp[i] = fopen(path, "rb")))
        {
            rc = error(IOERROR, "failed to open db");
            break;
        }
        db->nfiles++;
    }
    return rc;
}

enum rc db_handle_open(struct db_handle *db, char path[DCP_PATH_SIZE],
                           unsigned nfiles)
{
    enum rc rc = DONE;
    assert(nfiles > 0);
    if (db->nfiles == 0)
    {
        if ((rc = open_files(db, 0, 1, path))) goto cleanup;
        if ((rc = dcp_pro_db_openr(&db->pro, db->fp[0]))) goto cleanup;
    }
    else
        db_rewind(&db->pro.super);

    int num_files = (int)xmath_min(db_nprofiles(&db->pro.super), nfiles);
    int n = num_files - db->nfiles;
    if (n < 0 && (rc = close_files(db, db->nfiles + n, db->nfiles, false)))
        goto cleanup;

    if (n > 0 && (rc = open_files(db, db->nfiles, db->nfiles + n, path)))
        goto cleanup;

    rc = dcp_pro_db_setup_multi_readers(&db->pro, (unsigned)db->nfiles, db->fp);
    if (rc) goto cleanup;
    db_rewind(&db->pro.super);

    db->open_since = utc_now();
    return rc;

cleanup:
    close_files(db, 0, db->nfiles, true);
    return rc;
}

enum rc db_handle_close(struct db_handle *db)
{
    db->open_since = 0;
    enum rc rc = dcp_pro_db_close(&db->pro);
    if (rc) return rc;

    if ((rc = close_files(db, 0, db->nfiles, false))) goto cleanup;
    return rc;

cleanup:
    close_files(db, 0, db->nfiles, true);
    return rc;
}
