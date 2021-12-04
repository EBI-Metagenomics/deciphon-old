#include "dcp/dcp.h"
#include <stdio.h>

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size);

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    imm_log_setup(imm_log_default_callback, IMM_FATAL);

    FILE *fd = fmemopen((void *)data, size, "r");
    if (!fd)
        return 0;

    struct db *db = db_openr(fd);
    if (!db)
    {
        fclose(fd);
        return 0;
    }

    ;
    struct dcp_profile prof = DCP_PROFILE_INIT(db_abc(db));
    while (!db_end(db))
    {
        if (db_read(db, &prof))
            break;
    }

    dcp_profile_deinit(&prof);
    fclose(fd);
    return 0;
}
