#include "deciphon/core/xfile.h"
#include "ctb/ctb.h"
#include "deciphon/core/compiler.h"
#include "deciphon/core/logging.h"
#include "sched/structs.h"
#define XXH_INLINE_ALL
#include "xxhash/xxhash.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#ifndef PATH_MAX
#define PATH_MAX 4096
#endif

#ifdef __APPLE__
#include <fcntl.h>
#include <sys/syslimits.h>
#endif

#define BUFFSIZE (8 * 1024)

enum rc xfile_size(char const *filepath, int64_t *size)
{
    struct stat st = {0};
    if (stat(filepath, &st) == 1) return eio("stat");
    assert(sizeof(st.st_size) == 8);
    off_t sz = st.st_size;
    *size = (int64_t)sz;
    return RC_OK;
}

enum rc xfile_psize(FILE *fp, int64_t *size)
{
    off_t old = ftello(fp);
    if (old == -1) return eio("ftello");
    if (fseeko(fp, 0, SEEK_END) == -1) return eio("fseeko");
    off_t sz = ftello(fp);
    if (sz == -1) return eio("fseeko");
    if (fseeko(fp, old, SEEK_SET) == -1) return eio("fseeko");
    *size = (int64_t)sz;
    return RC_OK;
}

enum rc xfile_dsize(int fd, int64_t *size)
{
    struct stat st = {0};
    if (fsync(fd) == -1) return eio("fsync");
    if (fstat(fd, &st) == 1) return eio("fstat");
    assert(sizeof(st.st_size) == 8);
    off_t sz = st.st_size;
    *size = (int64_t)sz;
    return RC_OK;
}

static_assert(SAME_TYPE(XXH64_hash_t, uint64_t), "XXH64_hash_t is uint64_t");

enum rc xfile_hash(FILE *restrict fp, int64_t *hash)
{
    enum rc rc = RC_EFAIL;
    XXH3_state_t *state = XXH3_createState();
    if (!state)
    {
        rc = efail("failed to create state");
        goto cleanup;
    }
    XXH3_64bits_reset(state);

    size_t n = 0;
    unsigned char buffer[BUFFSIZE] = {0};
    while ((n = fread(buffer, sizeof(*buffer), BUFFSIZE, fp)) > 0)
    {
        if (n < BUFFSIZE && ferror(fp))
        {
            rc = eio("fread");
            goto cleanup;
        }

        XXH3_64bits_update(state, buffer, n);
    }
    if (ferror(fp))
    {
        rc = eio("fread");
        goto cleanup;
    }
    rc = RC_OK;

    union
    {
        int64_t const i;
        uint64_t const u;
    } const h = {.u = XXH3_64bits_digest(state)};
    *hash = h.i;

cleanup:
    XXH3_freeState(state);
    return rc;
}

enum rc xfile_tmp_open(struct xfile_tmp *file)
{
    CTB_STRLCPY(file, path, XFILE_PATH_TEMP_TEMPLATE);
    file->fp = 0;

    enum rc rc = xfile_mktemp(file->path);
    if (rc) return rc;

    if (!(file->fp = fopen(file->path, "wb+"))) return eio("fopen");

    return RC_OK;
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
        if (n < BUFFSIZE && ferror(src)) return eio("fread");

        if (fwrite(buffer, sizeof(*buffer), n, dst) < n) return eio("fwrite");
    }
    if (ferror(src)) return eio("fread");

    return RC_OK;
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
    if (mkstemp(filepath) == -1) return eio("mkstemp failed");
    return RC_OK;
}

int64_t xfile_tell(FILE *restrict fp) { return (int64_t)ftello(fp); }

bool xfile_seek(FILE *restrict fp, int64_t offset, int whence)
{
    return fseeko(fp, (off_t)offset, whence) != -1;
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
    if (n + 1 + (size_t)(j - str) > max_size) return enomem("allocate memory");
    *(j++) = *(ext++);
    *(j++) = *(ext++);
    *(j++) = *(ext++);
    *(j++) = *(ext++);
    *j = *ext;
    return RC_OK;
}

static enum rc change_ext(char *str, size_t pos, size_t max_size,
                          char const *ext)
{
    char *j = &str[pos];
    while (j > str && *j != '.')
        --j;
    if (j == str) return RC_EFAIL;
    return append_ext(str, (size_t)(j - str), max_size, ext);
}

enum rc xfile_set_ext(size_t max_size, char *str, char const *ext)
{
    size_t len = strlen(str);
    if (change_ext(str, len, max_size, ext))
        return append_ext(str, len, max_size, ext);
    return RC_OK;
}

void xfile_basename(char *filename, char const *path, size_t size)
{
    char *p = glibc_basename(path);
    ctb_strlcpy(filename, p, size);
}

void xfile_strip_ext(char *str)
{
    char *ret = strrchr(str, '.');
    if (ret) *ret = 0;
}

enum rc xfile_filepath_from_fptr(FILE *fp, char *filepath)
{
    int fd = fileno(fp);
#ifdef __APPLE__
    if (fcntl(fd, F_GETPATH, filepath) == -1) return eio("F_GET_PATH");
#else
    sprintf(filepath, "/proc/self/fd/%d", fd);
#endif
    return RC_OK;
}

FILE *xfile_open_from_fptr(FILE *fp, char const *mode)
{
    char filepath[PATH_MAX] = {0};
    enum rc rc = xfile_filepath_from_fptr(fp, filepath);
    if (rc) return NULL;
    return fopen(filepath, mode);
}

bool xfile_exists(char const *filepath) { return access(filepath, F_OK) == 0; }

enum rc xfile_touch(char const *filepath)
{
    if (xfile_exists(filepath)) return RC_OK;
    FILE *fp = fopen(filepath, "wb");
    if (!fp) return eio("fopen");
    if (fclose(fp)) return eio("fclose");
    return RC_OK;
}
