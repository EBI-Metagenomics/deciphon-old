#include "progress_file.h"
#include "error.h"
#include <assert.h>
#include <sys/stat.h>

static int filesize(FILE *file, off_t *size)
{
    int fd = fileno(file);
    struct stat st;
    int rc = fstat(fd, &st);
    *size = st.st_size;
    return rc;
}

void progress_file_init(struct progress_file *p, FILE *fd)
{
    p->enabled = true;
    p->at = ATHR_INIT;
    p->pos = 0;
    p->fd = fd;
}

#define FTELL_FAILED_MSG "failed to ftell for progress display"

void progress_file_start(struct progress_file *p, bool enabled)
{
    p->enabled = enabled;
    if (!p->enabled) return;

    if ((p->pos = ftell(p->fd)) == -1L)
    {
        error(DCP_IOERROR, FTELL_FAILED_MSG);
        p->enabled = false;
        return;
    }

    off_t fs = 0;
    if (filesize(p->fd, &fs))
    {
        error(DCP_IOERROR, "failed to get file size for progress display");
        p->enabled = false;
        return;
    }

    assert(fs >= p->pos);
    uint64_t total = (uint64_t)(fs - p->pos);

    if (athr_start(&p->at, total, "Press", ATHR_BAR | ATHR_PERC | ATHR_ETA))
    {
        error(DCP_IOERROR, "failed to athr_start for progress display");
        p->enabled = false;
    }
}

void progress_file_update(struct progress_file *p)
{
    if (!p->enabled) return;

    long curr_pos = ftell(p->fd);
    if (curr_pos == -1L)
    {
        error(DCP_IOERROR, FTELL_FAILED_MSG);
        p->enabled = false;
        athr_stop(&p->at);
        return;
    }

    assert(curr_pos >= p->pos);
    uint64_t progress = (uint64_t)(curr_pos - p->pos);
    p->pos = curr_pos;

    athr_eat(&p->at, progress);
}

void progress_file_stop(struct progress_file *p)
{
    if (p->enabled) athr_stop(&p->at);
}
