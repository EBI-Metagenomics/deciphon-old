#include "db_tmp.h"
#include "dcp/compiler.h"
#include "logger.h"

void db_tmp_setup(struct db_tmp *db)
{
    for (unsigned i = 0; i < ARRAY_SIZE_OF(*db, cmps); ++i)
        cmp_setup(&db->cmps[i], 0);
}

enum rc db_tmp_init(struct db_tmp *db)
{
    enum rc rc = DCP_OK;
    for (unsigned i = 0; i < ARRAY_SIZE_OF(*db, cmps); ++i)
    {
        FILE *fp = tmpfile();
        if (!fp)
        {
            rc = eio("create tmpfile");
            goto cleanup;
        }
        cmp_setup(&db->cmps[i], fp);
    }
    return rc;

cleanup:
    db_tmp_close(db);
    return rc;
}

void db_tmp_close(struct db_tmp *db)
{
    for (unsigned i = 0; i < ARRAY_SIZE_OF(*db, cmps); ++i)
    {
        if (cmp_file(db->cmps + i)) fclose(cmp_file(db->cmps + i));
    }
}
