#include "prod_file.h"
#include "dcp/rc.h"
#include "error.h"
#include "macros.h"
#include "xfile.h"
#include "xstrlcpy.h"

enum dcp_rc prod_file_open(struct prod_file *file)
{
    xstrlcpy(file->path, PATH_TEMP_TEMPLATE, MEMBER_SIZE(*file, path));
    file->fd = NULL;
    enum dcp_rc rc = xfile_mktemp(file->path);
    if (rc) return rc;
    if (!(file->fd = fopen(file->path, "wb")))
        rc = error(DCP_IOERROR, "failed to open prod file");
    return rc;
}

enum dcp_rc prod_file_write_sep(struct prod_file *file)
{
    if (fputc('\t', file->fd) == EOF)
        return error(DCP_IOERROR, "failed to write sep");
    return DCP_DONE;
}

enum dcp_rc prod_file_write_nl(struct prod_file *file)
{
    if (fputc('\n', file->fd) == EOF)
        return error(DCP_IOERROR, "failed to write nl");
    return DCP_DONE;
}

enum dcp_rc prod_file_close(struct prod_file *file)
{
    if (fclose(file->fd)) return error(DCP_IOERROR, "failed to close file");
    if (remove(file->path)) return error(DCP_IOERROR, "failed to remove file");
    return DCP_DONE;
}
