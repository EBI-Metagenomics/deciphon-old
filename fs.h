#ifndef FS_H
#define FS_H

#include <stdio.h>

int fs_tell(FILE *restrict fp, long *offset);
int fs_seek(FILE *restrict fp, long offset, int whence);
int fs_copy(FILE *restrict dst, FILE *restrict src);
int fs_refopen(FILE *fp, char const *mode, FILE **out);
int fs_getpath(FILE *fp, unsigned size, char *filepath);
int fs_close(FILE *fp);

#endif
