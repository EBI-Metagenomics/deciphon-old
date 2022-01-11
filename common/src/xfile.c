#include "common/xfile.h"
#include "common/compiler.h"
#include "common/logger.h"
#include "common/safe.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <xxhash.h>

#define BUFFSIZE (8 * 1024)

static_assert(same_type(XXH64_hash_t, uint64_t), "XXH64_hash_t is uint64_t");

enum rc xfile_hash(FILE *restrict fp, uint64_t *hash)
{
    int rc = RC_FAIL;
    XXH64_state_t *const state = XXH64_createState();
    if (!state)
    {
        rc = error(RC_FAIL, "failed to create state");
        goto cleanup;
    }
    XXH64_reset(state, 0);

    size_t n = 0;
    unsigned char buffer[BUFFSIZE] = {0};
    while ((n = fread(buffer, sizeof(*buffer), BUFFSIZE, fp)) > 0)
    {
        if (n < BUFFSIZE && ferror(fp))
        {
            rc = failed_to(RC_IOERROR, "fread");
            goto cleanup;
        }

        XXH64_update(state, buffer, n);
    }
    if (ferror(fp))
    {
        rc = failed_to(RC_IOERROR, "fread");
        goto cleanup;
    }

    rc = RC_DONE;
    *hash = XXH64_digest(state);

cleanup:
    XXH64_freeState(state);
    return rc;
}

enum rc xfile_tmp_open(struct xfile_tmp *file)
{
    safe_strcpy(file->path, XFILE_PATH_TEMP_TEMPLATE,
                ARRAY_SIZE_OF(*file, path));
    file->fp = 0;

    enum rc rc = xfile_mktemp(file->path);
    if (rc) return rc;

    if (!(file->fp = fopen(file->path, "wb+"))) return failed_to(RC_IOERROR, "fopen");

    return RC_DONE;
}

void xfile_tmp_del(struct xfile_tmp const *file)
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
        if (n < BUFFSIZE && ferror(src)) return failed_to(RC_IOERROR, "fread");

        if (fwrite(buffer, sizeof(*buffer), n, dst) < n) return failed_to(RC_IOERROR, "fwrite");
    }
    if (ferror(src)) return failed_to(RC_IOERROR, "fread");

    return RC_DONE;
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
    if (mkstemp(filepath) == -1) return error(RC_IOERROR, "mkstemp failed");
    return RC_DONE;
}

static char *glibc_basename(const char *filename)
{
    char *p = strrchr(filename, '/');
    return p ? p + 1 : (char *)filename;
}

static enum rc append_ext(char *str, size_t len, size_t max_size,
                          char const *ext)
{
    char *j = &str[len];
    size_t n = strlen(ext);
    if (n + 1 + (size_t)(j - str) > max_size)
        return error(RC_OUTOFMEM, "not enough memory");
    *(j++) = *(ext++);
    *(j++) = *(ext++);
    *(j++) = *(ext++);
    *(j++) = *(ext++);
    *j = *ext;
    return RC_DONE;
}

static enum rc change_ext(char *str, size_t pos, size_t max_size,
                          char const *ext)
{
    char *j = &str[pos];
    while (j > str && *j != '.')
        --j;
    if (j == str) return RC_FAIL;
    return append_ext(str, (size_t)(j - str), max_size, ext);
}

enum rc xfile_set_ext(size_t max_size, char *str, char const *ext)
{
    size_t len = strlen(str);
    if (change_ext(str, len, max_size, ext))
        return append_ext(str, len, max_size, ext);
    return RC_DONE;
}

void xfile_basename(char *filename, char const *path)
{
    char *p = glibc_basename(path);
    safe_strcpy(filename, p, DCP_FILENAME_SIZE);
}

void xfile_strip_ext(char *str)
{
    char *ret = strrchr(str, '.');
    if (ret) *ret = 0;
}

FILE *xfile_open_from_fptr(FILE *fp, char const *mode)
{
    int fd = fileno(fp);
    return fdopen(fd, "rb");
}
