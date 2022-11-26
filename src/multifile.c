#include "multifile.h"
#include "deciphon_limits.h"
#include "fs.h"
#include "logy.h"
#include "sizeof_field.h"
#include <stdio.h>
#include <string.h>

void multifile_init(struct multifile *mf) { multifile_cleanup(mf); }

int multifile_setup(struct multifile *mf, int size)
{
    for (mf->size = 0; mf->size < size; ++mf->size)
    {
        if (!(mf->files[mf->size] = tmpfile()))
        {
            multifile_cleanup(mf);
            return eio("failed to create temporary files");
        }
    }
    return RC_OK;
}

FILE *multifile_file(struct multifile *mf, int idx) { return mf->files[idx]; }

static void cleanup_files(struct multifile *);
static int setup_final_file(struct multifile *);
static void cleanup_final_file(struct multifile *);
static int join_files(struct multifile *);

int multifile_finishup(struct multifile *mf, char const **path)
{
    int rc = setup_final_file(mf);
    if (rc) return rc;
    rc = join_files(mf);
    if (rc) return rc;
    cleanup_files(mf);
    *path = mf->final.path;
    return RC_OK;
}

void multifile_cleanup(struct multifile *mf)
{
    cleanup_files(mf);
    cleanup_final_file(mf);
}

static int join_files(struct multifile *mf)
{
    for (int i = 0; i < mf->size; ++i)
    {
        if (fflush(mf->files[i])) return eio("failed to flush");
        rewind(mf->files[i]);
        int r = fs_copy_fp(mf->final.file, mf->files[i]);
        if (r) return eio("%s", fs_strerror(r));
    }
    if (fflush(mf->final.file)) return eio("failed to flush");

    return RC_OK;
}

static void cleanup_files(struct multifile *mf)
{
    for (int i = 0; i < mf->size; ++i)
    {
        fclose(mf->files[i]);
        mf->files[i] = NULL;
    }
    mf->size = 0;
}

static int setup_final_file(struct multifile *mf)
{
    size_t size = sizeof_field(struct multifile_final, path);
    mf->final.file = NULL;
    if (fs_mkstemp(size, mf->final.path)) return eio("fail to finish product");

    mf->final.file = fopen(mf->final.path, "wb");
    return mf->final.file ? RC_OK : eio("fail to finish product");
}

static void cleanup_final_file(struct multifile *mf)
{
    if (mf->final.file)
    {
        fclose(mf->final.file);
        mf->final.file = NULL;
    }

    if (*mf->final.path && fs_exists(mf->final.path)) fs_unlink(mf->final.path);

    memset(mf->final.path, '\0', sizeof_field(struct multifile_final, path));
}
