#include "core/thrfiles.h"
#include "core/limits.h"
#include "core/logy.h"
#include "core/pp.h"
#include "fs.h"
#include <stdio.h>
#include <string.h>

void thrfiles_init(struct thrfiles *tf) { thrfiles_cleanup(tf); }

bool thrfiles_setup(struct thrfiles *tf, int nthreads)
{
    for (tf->size = 0; tf->size < nthreads; ++tf->size)
    {
        if (!(tf->files[tf->size] = tmpfile()))
        {
            thrfiles_cleanup(tf);
            return !eio("failed to create temporary files");
        }
    }
    return false;
}

static void cleanup_thread_files(struct thrfiles *);
static void cleanup_final_file(struct thrfiles *);
static bool join_thread_files(struct thrfiles *);

char const *tf_finishup(struct thrfiles *tf)
{
    join_thread_files(tf);
    cleanup_thread_files(tf);
    return tf->final.path;
}

void thrfiles_cleanup(struct thrfiles *tf)
{
    cleanup_thread_files(tf);
    cleanup_final_file(tf);
}

static bool join_thread_files(struct thrfiles *tf)
{
    for (int i = 0; i < tf->size; ++i)
    {
        if (fflush(tf->files[i]))
        {
            eio("failed to flush");
            goto cleanup;
        }
        rewind(tf->files[i]);
        int r = fs_copy_fp(tf->final.file, tf->files[i]);
        if (r)
        {
            eio("%s", fs_strerror(r));
            goto cleanup;
        }
    }
    if (fflush(tf->final.file))
    {
        eio("failed to flush");
        goto cleanup;
    }

    return true;

cleanup:
    return false;
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
