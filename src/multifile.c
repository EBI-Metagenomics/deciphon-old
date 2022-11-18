#include "multifile.h"
#include "core/limits.h"
#include "fs.h"
#include "logy.h"
#include "sizeof_field.h"
#include <stdio.h>
#include <string.h>

void multifile_init(struct multifile *tf) { multifile_cleanup(tf); }

int multifile_setup(struct multifile *tf, int size)
{
    for (tf->size = 0; tf->size < size; ++tf->size)
    {
        if (!(tf->files[tf->size] = tmpfile()))
        {
            multifile_cleanup(tf);
            return eio("failed to create temporary files");
        }
    }
    return RC_OK;
}

static void cleanup_files(struct multifile *);
static int setup_final_file(struct multifile *);
static void cleanup_final_file(struct multifile *);
static int join_files(struct multifile *);

int multifile_finishup(struct multifile *tf, char const **path)
{
    int rc = setup_final_file(tf);
    if (rc) return rc;
    rc = join_files(tf);
    if (rc) return rc;
    cleanup_files(tf);
    *path = tf->final.path;
    return RC_OK;
}

void multifile_cleanup(struct multifile *tf)
{
    cleanup_files(tf);
    cleanup_final_file(tf);
}

static int join_files(struct multifile *tf)
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

static void cleanup_files(struct multifile *tf)
{
    for (int i = 0; i < tf->size; ++i)
    {
        fclose(tf->files[i]);
        tf->files[i] = NULL;
    }
    tf->size = 0;
}

static int setup_final_file(struct multifile *tf)
{
    size_t size = sizeof_field(struct multifile_final, path);
    tf->final.file = NULL;
    if (fs_mkstemp(size, tf->final.path)) return eio("fail to finish product");

    tf->final.file = fopen(tf->final.path, "wb");
    return tf->final.file ? RC_OK : eio("fail to finish product");
}

static void cleanup_final_file(struct multifile *tf)
{
    if (tf->final.file)
    {
        fclose(tf->final.file);
        tf->final.file = NULL;
    }

    if (*tf->final.path && fs_exists(tf->final.path)) fs_unlink(tf->final.path);

    memset(tf->final.path, '\0', sizeof_field(struct multifile_final, path));
}
