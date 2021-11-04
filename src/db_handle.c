#include "db_handle.h"
#include "error.h"
#include "sched.h"

enum dcp_rc db_handle_setup(struct db_handle *db, struct sched *sched,
                            int64_t id)
{
    char filepath[PATH_SIZE] = {0};
    enum dcp_rc rc = DCP_DONE;
    if ((rc = sched_db_filepath(sched, id, filepath))) return rc;

    if (!(db->fd = fopen(filepath, "rb")))
        return error(DCP_IOERROR, "failed to open db file");

    if ((rc = dcp_pro_db_openr(&db->pro, db->fd))) fclose(db->fd);

    return rc;
}
