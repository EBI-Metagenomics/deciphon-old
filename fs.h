#ifndef FS_H
#define FS_H

#include <stdbool.h>
#include <stdio.h>

int fs_tell(FILE *restrict fp, long *offset);
int fs_seek(FILE *restrict fp, long offset, int whence);
int fs_copy(FILE *restrict dst, FILE *restrict src);
int fs_refopen(FILE *fp, char const *mode, FILE **out);
int fs_getpath(FILE *fp, unsigned size, char *filepath);
int fs_close(FILE *fp);
int fs_readall(char const *filepath, long *size, unsigned char **data);
int fs_tmpfile(FILE **out);
int fs_copyp(FILE *restrict dst, FILE *restrict src);
int fs_cksum(char const *filepath, long *chk);
int fs_mkdir(char const *dirpath, bool exist_ok);
int fs_rmdir(char const *dirpath);
int fs_rmfile(char const *filepath);
int fs_touch(char const *filepath);

#endif
