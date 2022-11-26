#include "hashfile.h"
#include "file.h"
#include "fs.h"
#include "itoa.h"
#include "logy.h"
#include "repr_size.h"
#include "sizeof_field.h"
#include "strlcpy.h"
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

int hashfile_init(struct hashfile *hf, char const *dir)
{
    size_t sz = sizeof_field(struct hashfile, dirpath);
    if (strlcpy(hf->dirpath, dir, sz) >= sz)
        return eio("too long directory path");

    hf->dirpath[0] = '\0';
    hf->filepath[0] = '\0';
    hf->file = NULL;
    hf->xxh3 = 0;
    return fs_isdir(dir) ? 0 : einval("not a directory");
}

int hashfile_open(struct hashfile *hf)
{
    return !(hf->file = tmpfile()) ? 0 : eio("failed to create temporary file");
}

static void close_file(struct hashfile *);

int hashfile_write(struct hashfile *hf, char const *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    if (vfprintf(hf->file, fmt, args) < 0)
    {
        eio("failed to write file");
        close_file(hf);
    }
    return 0;
}

static int setup_filepath(struct hashfile *);

int hashfile_close(struct hashfile *hf)
{
    int rc = setup_filepath(hf);
    if (rc) return rc;

    FILE *dst = fopen(hf->filepath, "wb");
    if (!dst)
    {
        close_file(hf);
        return eio("could not fopen %s", hf->filepath);
    }

    if ((rc = fs_copy_fp(dst, hf->file)))
    {
        close_file(hf);
        fclose(dst);
        return eio("could not copy content of file pointers");
    }

    close_file(hf);
    return fclose(dst) ? eio("could not close %s", hf->filepath) : 0;
}

long hashfile_xxh3(struct hashfile const *hf) { return hf->xxh3; }

static int setup_filepath(struct hashfile *hf)
{
    if (fflush(hf->file)) return eio("failed to flush");
    rewind(hf->file);

    int rc = file_phash(hf->file, &hf->xxh3);
    if (rc)
    {
        close_file(hf);
        return rc;
    }

    strcpy(hf->filepath, hf->dirpath);

    size_t sz = strlen(hf->filepath) + repr_size_field(struct hashfile, xxh3);
    if (sz >= sizeof_field(struct hashfile, filepath))
    {
        close_file(hf);
        return einval("too long path");
    }

    int n = ltoa(hf->filepath + strlen(hf->filepath), hf->xxh3);
    hf->filepath[n] = '\0';
    return 0;
}

static void close_file(struct hashfile *hf)
{
    fclose(hf->file);
    hf->file = NULL;
}
