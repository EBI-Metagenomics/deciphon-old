#include "work.h"
#include "error.h"
#include "xstrlcpy.h"
#include "xfile.h"
#include "dcp.h"

static enum dcp_rc prod_file_open(struct prod_file *file)
{
    xstrlcpy(file->path, PATH_TEMP_TEMPLATE, MEMBER_SIZE(*file, path));
    enum dcp_rc rc = xfile_mktemp(file->path);
    if (rc) return rc;
    if (!(file->fd = fopen(file->path, "wb")))
        rc = error(DCP_IOERROR, "failed to open prod file");
    return rc;
}
