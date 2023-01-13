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
#include "rc.h"
#include <assert.h>
#include <limits.h>
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
  return (*offset = ftello(fp)) < 0 ? EFTELL : 0;
}

int fs_seek(FILE *restrict fp, long offset, int whence)
{
  return fseeko(fp, (off_t)offset, whence) < 0 ? EFSEEK : 0;
}

int fs_copy(FILE *restrict dst, FILE *restrict src)
{
  static _Thread_local char buffer[BUFFSIZE];
  size_t n = 0;
  while ((n = fread(buffer, sizeof(*buffer), BUFFSIZE, src)) > 0)
  {
    if (n < BUFFSIZE && ferror(src)) return EFREAD;

    if (fwrite(buffer, sizeof(*buffer), n, dst) < n) return EFWRITE;
  }
  if (ferror(src)) return EFREAD;

  return 0;
}

int fs_refopen(FILE *fp, char const *mode, FILE **out)
{
  char filepath[FILENAME_MAX] = {0};
  int rc = fs_getpath(fp, sizeof filepath, filepath);
  if (rc) return rc;
  return (*out = fopen(filepath, mode)) ? 0 : EFOPEN;
}

int fs_getpath(FILE *fp, unsigned size, char *filepath)
{
  int fd = fileno(fp);
  if (fd < 0) return EGETPATH;

#ifdef __APPLE__
  (void)size;
  char pathbuf[MAXPATHLEN] = {0};
  if (fcntl(fd, F_GETPATH, pathbuf) < 0) return EGETPATH;
  if (strlen(pathbuf) >= size) return ETRUNCPATH;
  strcpy(filepath, pathbuf);
#else
  char pathbuf[FILENAME_MAX] = {0};
  sprintf(pathbuf, "/proc/self/fd/%d", fd);
  ssize_t n = readlink(pathbuf, filepath, size);
  if (n < 0) return EGETPATH;
  if (n >= size) return ETRUNCPATH;
  filepath[n] = '\0';
#endif

  return 0;
}

int fs_close(FILE *fp) { return fclose(fp) ? EFCLOSE : 0; }
