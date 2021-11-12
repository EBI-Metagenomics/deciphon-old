#include "xfile.h"
#include "error.h"
#include "macros.h"
#include "xstrlcpy.h"
#include <assert.h>
#include <unistd.h>

#ifndef __USE_XOPEN_EXTENDED
/* To make mkstemp available. */
#define __USE_XOPEN_EXTENDED
#endif
#include <stdlib.h>

#define BUFFSIZE (8 * 1024)

enum dcp_rc xfile_tmp_open(struct xfile_tmp *file)
{
    xstrlcpy(file->path, PATH_TEMP_TEMPLATE, MEMBER_SIZE(*file, path));
    file->fd = NULL;
    enum dcp_rc rc = xfile_mktemp(file->path);
    if (rc) return rc;
    if (!(file->fd = fopen(file->path, "wb+")))
        rc = error(DCP_IOERROR, "failed to open prod file");
    return rc;
}

enum dcp_rc xfile_tmp_rewind(struct xfile_tmp *file)
{
    if (fflush(file->fd)) return error(DCP_IOERROR, "failed to flush file");
    rewind(file->fd);
    return DCP_DONE;
}

void xfile_tmp_destroy(struct xfile_tmp *file)
{
    fclose(file->fd);
    remove(file->path);
}

enum dcp_rc xfile_copy(FILE *restrict dst, FILE *restrict src)
{
    char buffer[BUFFSIZE];
    size_t n = 0;
    while ((n = fread(buffer, sizeof(*buffer), BUFFSIZE, src)) > 0)
    {
        if (n < BUFFSIZE && ferror(src))
            return error(DCP_IOERROR, "failed to read file");

        if (fwrite(buffer, sizeof(*buffer), n, dst) < n)
            return error(DCP_IOERROR, "failed to write file");
    }
    if (ferror(src)) return error(DCP_IOERROR, "failed to read file");

    return DCP_DONE;
}

bool xfile_is_readable(char const filepath[PATH_SIZE])
{
    FILE *file = NULL;
    if ((file = fopen(filepath, "r")))
    {
        fclose(file);
        return true;
    }
    return false;
}

enum dcp_rc xfile_mktemp(char filepath[PATH_SIZE])
{
    if (mkstemp(filepath) == -1) return error(DCP_IOERROR, "mkstemp failed");
    return DCP_DONE;
}
