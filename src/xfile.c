#include "xfile.h"
#include "logger.h"
#include "macros.h"
#include "safe.h"
#include <assert.h>
#include <unistd.h>
#include <xxhash.h>

#ifndef __USE_XOPEN_EXTENDED
/* To make mkstemp available. */
#define __USE_XOPEN_EXTENDED
#endif
#include <stdlib.h>

#define BUFFSIZE (8 * 1024)

enum rc xfile_tmp_open(struct xfile_tmp *file)
{
    safe_strcpy(file->path, PATH_TEMP_TEMPLATE, MEMBER_SIZE(*file, path));
    file->fp = NULL;
    enum rc rc = xfile_mktemp(file->path);
    if (rc) return rc;
    if (!(file->fp = fopen(file->path, "wb+")))
        rc = error(IOERROR, "failed to open prod file");
    return rc;
}

enum rc xfile_tmp_rewind(struct xfile_tmp *file)
{
    if (fflush(file->fp)) return error(IOERROR, "failed to flush file");
    rewind(file->fp);
    return DONE;
}

void xfile_tmp_destroy(struct xfile_tmp *file)
{
    fclose(file->fp);
    remove(file->path);
}

enum rc xfile_copy(FILE *restrict dst, FILE *restrict src)
{
    char buffer[BUFFSIZE];
    size_t n = 0;
    while ((n = fread(buffer, sizeof(*buffer), BUFFSIZE, src)) > 0)
    {
        if (n < BUFFSIZE && ferror(src))
            return error(IOERROR, "failed to read file");

        if (fwrite(buffer, sizeof(*buffer), n, dst) < n)
            return error(IOERROR, "failed to write file");
    }
    if (ferror(src)) return error(IOERROR, "failed to read file");

    return DONE;
}

bool xfile_is_readable(char const *filepath)
{
    FILE *file = NULL;
    if ((file = fopen(filepath, "r")))
    {
        fclose(file);
        return true;
    }
    return false;
}

enum rc xfile_mktemp(char *filepath)
{
    if (mkstemp(filepath) == -1) return error(IOERROR, "mkstemp failed");
    return DONE;
}

/* Are two types/vars the same type (ignoring qualifiers)? */
#define same_type(a, b) __builtin_types_compatible_p(typeof(a), typeof(b))

static_assert(same_type(XXH64_hash_t, uint64_t), "XXH64_hash_t is uint64_t");

enum rc xfile_hash(FILE *restrict fp, uint64_t *hash)
{
    enum rc rc = DONE;
    XXH64_state_t *const state = XXH64_createState();
    if (!state)
    {
        rc = error(OUTOFMEM, "not enough memory for hashing");
        goto cleanup;
    }
    XXH64_reset(state, 0);

    size_t n = 0;
    unsigned char buffer[BUFFSIZE] = {0};
    while ((n = fread(buffer, sizeof(*buffer), BUFFSIZE, fp)) > 0)
    {
        if (n < BUFFSIZE && ferror(fp))
        {
            rc = error(IOERROR, "failed to read file");
            goto cleanup;
        }

        XXH64_update(state, buffer, n);
    }
    if (ferror(fp))
    {
        rc = error(IOERROR, "failed to read file");
        goto cleanup;
    }

    *hash = XXH64_digest(state);

cleanup:
    XXH64_freeState(state);
    return rc;
}
