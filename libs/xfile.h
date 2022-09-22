#ifndef XFILE_H
#define XFILE_H

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#define XFILE_MAP(X)                                                           \
    X(OK, "not an error")                                                      \
    X(EFCLOSE, "fclose failure")                                               \
    X(EFCNTL, "fcntl failure")                                                 \
    X(EFILENO, "fileno failure")                                               \
    X(EFOPEN, "fopen failure")                                                 \
    X(EFREAD, "fread failure")                                                 \
    X(EFSEEK, "fseek failure")                                                 \
    X(EFSTAT, "fstat failure")                                                 \
    X(EFSYNC, "fsync failure")                                                 \
    X(EFTELL, "ftell failure")                                                 \
    X(EFWRITE, "fwrite failure")                                               \
    X(ENOMEM, "not enough memory")                                             \
    X(EREADLINK, "readlink failure")                                           \
    X(ERMDIR, "rmdir failure")                                                 \
    X(ESTAT, "stat failure")                                                   \
    X(ETRUNCPATH, "truncated path")                                            \
    X(EUNLINK, "unlink failure")

enum xfile_rc
{
#define X(A, _) XFILE_##A,
    XFILE_MAP(X)
#undef X
};

int xfile_size(char const *filepath, int64_t *size);
int xfile_psize(FILE *fp, int64_t *size);
int xfile_dsize(int fd, int64_t *size);

int xfile_tell(FILE *restrict fp, int64_t *offset);
int xfile_seek(FILE *restrict fp, int64_t offset, int whence);

int xfile_copy(FILE *restrict dst, FILE *restrict src);
int xfile_unlink(char const *filepath);
int xfile_rmdir(char const *dirpath);

int xfile_refopen(FILE *fp, char const *mode, FILE **out);
int xfile_fileno(FILE *fp, int *fd);
int xfile_getpath(FILE *fp, unsigned size, char *filepath);

bool xfile_exists(char const *filepath);
int xfile_touch(char const *filepath);

int xfile_readall(char const *filepath, int64_t *size, unsigned char **data);

char const *xfile_strerror(int rc);

#endif
