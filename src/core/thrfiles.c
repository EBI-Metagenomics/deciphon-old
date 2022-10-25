#include "core/thrfiles.h"
#include "core/limits.h"
#include "core/logy.h"
#include "core/pp.h"
#include "fs.h"
#include <stdio.h>
#include <string.h>

void thrfiles_init(struct thrfiles *tf) { thrfiles_cleanup(tf); }

int thrfiles_setup(struct thrfiles *tf, int nthreads)
{
    for (tf->size = 0; tf->size < nthreads; ++tf->size)
    {
        if (!(tf->files[tf->size] = tmpfile()))
        {
            thrfiles_cleanup(tf);
            return eio("failed to create temporary files");
        }
    }
    return RC_OK;
}

static void cleanup_thread_files(struct thrfiles *);
static int setup_final_open(struct thrfiles *);
static void cleanup_final_file(struct thrfiles *);
static int join_thread_files(struct thrfiles *);

int thrfiles_finishup(struct thrfiles *tf, char const **path)
{
    int rc = setup_final_open(tf);
    if (rc) return rc;
    rc = join_thread_files(tf);
    if (rc) return rc;
    cleanup_thread_files(tf);
    *path = tf->final.path;
    return RC_OK;
}

void thrfiles_cleanup(struct thrfiles *tf)
{
    cleanup_thread_files(tf);
    cleanup_final_file(tf);
}

static int join_thread_files(struct thrfiles *tf)
{
    for (int i = 0; i < tf->size; ++i)
    {
        if (fflush(tf->files[i])) return eio("failed to flush");
        rewind(tf->files[i]);
        int r = fs_copy_fp(tf->final.file, tf->files[i]);
        if (r) return eio("%s", fs_strerror(r));
    }
    if (fflush(tf->final.file)) return eio("failed to flush");

    return RC_OK;
}

static void cleanup_thread_files(struct thrfiles *tf)
{
    for (int i = 0; i < tf->size; ++i)
    {
        fclose(tf->files[i]);
        tf->files[i] = NULL;
    }
    tf->size = 0;
}

static int setup_final_open(struct thrfiles *tf)
{
    size_t size = sizeof_field(struct thrfiles_final, path);
    tf->final.file = NULL;
    if (fs_mkstemp(size, tf->final.path)) return eio("fail to finish product");

    tf->final.file = fopen(tf->final.path, "wb");
    return tf->final.file ? RC_OK : eio("fail to finish product");
}

static void cleanup_final_file(struct thrfiles *tf)
{
    if (tf->final.file)
    {
        fclose(tf->final.file);
        tf->final.file = NULL;
    }

    if (*tf->final.path && fs_exists(tf->final.path)) fs_unlink(tf->final.path);

    memset(tf->final.path, '\0', sizeof_field(struct thrfiles_final, path));
}
