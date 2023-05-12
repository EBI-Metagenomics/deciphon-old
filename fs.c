#if !defined(_POSIX_C_SOURCE) || _POSIX_C_SOURCE < 200809L
#undef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 200809L
#endif

#if !defined(_FILE_OFFSET_BITS) || _FILE_OFFSET_BITS < 64
#undef _FILE_OFFSET_BITS
#define _FILE_OFFSET_BITS 64
#endif

#include "fs.h"
#include "array_size.h"
#include "deciphon/errno.h"
#include <assert.h>
#include <errno.h>
#include <limits.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#if defined(__APPLE__)
#if defined(_DARWIN_C_SOURCE)
#undef _DARWIN_C_SOURCE
#endif
#define _DARWIN_C_SOURCE 1
#include <fcntl.h>
#include <sys/param.h>
#endif

#if defined(__APPLE__) || defined(__FreeBSD__)
#include <copyfile.h>
#else
#include <fcntl.h>
#include <sys/sendfile.h>
#endif

#define BUFFSIZE (8 * 1024)

int fs_tell(FILE *restrict fp, long *offset)
{
  return (*offset = ftello(fp)) < 0 ? DCP_EFTELL : 0;
}

int fs_seek(FILE *restrict fp, long offset, int whence)
{
  return fseeko(fp, (off_t)offset, whence) < 0 ? DCP_EFSEEK : 0;
}

int fs_copy(FILE *restrict dst, FILE *restrict src)
{
  static _Thread_local char buffer[BUFFSIZE];
  size_t n = 0;
  while ((n = fread(buffer, sizeof(*buffer), BUFFSIZE, src)) > 0)
  {
    if (n < BUFFSIZE && ferror(src)) return DCP_EFREAD;

    if (fwrite(buffer, sizeof(*buffer), n, dst) < n) return DCP_EFWRITE;
  }
  if (ferror(src)) return DCP_EFREAD;

  return 0;
}

int fs_refopen(FILE *fp, char const *mode, FILE **out)
{
  char filepath[FILENAME_MAX] = {0};
  int rc = fs_getpath(fp, sizeof filepath, filepath);
  if (rc) return rc;
  return (*out = fopen(filepath, mode)) ? 0 : DCP_EREFOPEN;
}

int fs_getpath(FILE *fp, unsigned size, char *filepath)
{
  int fd = fileno(fp);
  if (fd < 0) return DCP_EGETPATH;

#ifdef __APPLE__
  (void)size;
  char pathbuf[MAXPATHLEN] = {0};
  if (fcntl(fd, F_GETPATH, pathbuf) < 0) return DCP_EGETPATH;
  if (strlen(pathbuf) >= size) return DCP_ETRUNCPATH;
  strcpy(filepath, pathbuf);
#else
  char pathbuf[FILENAME_MAX] = {0};
  sprintf(pathbuf, "/proc/self/fd/%d", fd);
  ssize_t n = readlink(pathbuf, filepath, size);
  if (n < 0) return DCP_EGETPATH;
  if (n >= size) return DCP_ETRUNCPATH;
  filepath[n] = '\0';
#endif

  return 0;
}

int fs_close(FILE *fp) { return fclose(fp) ? DCP_EFCLOSE : 0; }

static int fs_size(char const *filepath, long *size)
{
  struct stat st = {0};
  if (stat(filepath, &st) == 1) return DCP_EFSTAT;
  *size = (long)st.st_size;
  return 0;
}

int fs_readall(char const *filepath, long *size, unsigned char **data)
{
  *size = 0;
  *data = NULL;
  int rc = fs_size(filepath, size);
  if (rc) return rc;

  if (*size == 0) return 0;

  FILE *fp = fopen(filepath, "rb");
  if (!fp) return DCP_EFOPEN;

  if (!(*data = malloc(*size)))
  {
    fclose(fp);
    return DCP_ENOMEM;
  }

  if (fread(*data, *size, 1, fp) < 1)
  {
    fclose(fp);
    free(*data);
    return DCP_EFREAD;
  }

  return fclose(fp) ? DCP_EFCLOSE : 0;
}

int fs_tmpfile(FILE **out) { return (*out = tmpfile()) ? 0 : DCP_ETMPFILE; }

int fs_copyp(FILE *restrict dst, FILE *restrict src)
{
  char buffer[8 * 1024];
  size_t n = 0;
  while ((n = fread(buffer, sizeof(*buffer), BUFFSIZE, src)) > 0)
  {
    if (n < BUFFSIZE && ferror(src)) return DCP_EFREAD;

    if (fwrite(buffer, sizeof(*buffer), n, dst) < n) return DCP_EFWRITE;
  }
  if (ferror(src)) return DCP_EFREAD;

  return 0;
}

static int fletcher16(FILE *fp, uint8_t *buf, size_t bufsize, long *chk)
{
  size_t n = 0;
  uint16_t sum1 = 0;
  uint16_t sum2 = 0;
  while ((n = fread(buf, 1, bufsize, fp)) > 0)
  {
    if (n < bufsize && ferror(fp)) return DCP_EFREAD;
    for (int i = 0; i < (int)n; ++i)
    {
      sum1 = (sum1 + buf[i]) % 255;
      sum2 = (sum2 + sum1) % 255;
    }
  }
  if (ferror(fp)) return DCP_EFREAD;

  *chk = (sum2 << 8) | sum1;
  return 0;
}

int fs_cksum(char const *filepath, long *chk)
{
  static _Thread_local uint8_t buffer[8 * 1024];
  FILE *fp = fopen(filepath, "rb");
  if (!fp) return DCP_EFOPEN;

  int rc = fletcher16(fp, buffer, sizeof(buffer), chk);
  fclose(fp);
  return rc;
}

int fs_mkdir(char const *x, bool exist_ok)
{
  return (mkdir(x, 0755) && !(errno == EEXIST && exist_ok)) ? DCP_EMKDIR : 0;
}

int fs_rmdir(char const *x) { return rmdir(x) < 0 ? DCP_ERMDIR : 0; }

int fs_rmfile(char const *x) { return unlink(x) < 0 ? DCP_ERMFILE : 0; }

int fs_touch(char const *x)
{
  if (access(x, F_OK) == 0) return 0;
  FILE *fp = fopen(x, "wb");
  if (!fp) return DCP_EFOPEN;
  return fclose(fp) ? DCP_EFCLOSE : 0;
}
