#include "db_handle.h"
#include "db.h"
#include "dcp/pro_db.h"
#include "error.h"
#include "utc.h"
#include "xstrlcpy.h"

void db_handle_init(struct db_handle *db, int64_t id)
{
    db->id = id;
    db->pool_id = 0;
    cco_hnode_init(&db->hnode);
    db->fd = NULL;
    db->open_since = 0;
}
enum dcp_rc db_handle_open(struct db_handle *db, char path[PATH_SIZE])
{
    if (db_handle_is_open(db)) return db_rewind(&db->pro.super);

    db->fd = fopen(path, "rb");
    if (!db->fd) return error(DCP_IOERROR, "failed to open db");

    enum dcp_rc rc = dcp_pro_db_openr(&db->pro, db->fd);
    if (rc)
    {
        fclose(db->fd);
        return rc;
    }

    db->open_since = utc_now();
    return DCP_DONE;
}

enum dcp_rc db_handle_close(struct db_handle *db)
{
    db->open_since = 0;
    enum dcp_rc rc = dcp_pro_db_close(&db->pro);
    if (rc) return rc;
    if (fclose(db->fd)) return error(DCP_IOERROR, "failed to close db");
    return DCP_DONE;
}
