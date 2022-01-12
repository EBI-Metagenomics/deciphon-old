#ifndef XFILE_H
#define XFILE_H

#include "common/export.h"
#include "common/limits.h"
#include "rc.h"
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#define XFILE_PATH_TEMP_TEMPLATE "/tmp/dcpXXXXXX"

struct xfile_tmp
{
    char path[sizeof XFILE_PATH_TEMP_TEMPLATE];
    FILE *fp;
};

EXPORT enum rc xfile_hash(FILE *restrict fp, uint64_t *hash);

EXPORT enum rc xfile_tmp_open(struct xfile_tmp *file);
EXPORT void xfile_tmp_del(struct xfile_tmp const *file);

EXPORT enum rc xfile_copy(FILE *restrict dst, FILE *restrict src);
EXPORT bool xfile_is_readable(char const *filepath);
EXPORT enum rc xfile_mktemp(char *filepath);

EXPORT enum rc xfile_set_ext(size_t max_size, char *str, char const *ext);
EXPORT void xfile_basename(char *filename, char const *path);
EXPORT void xfile_strip_ext(char *str);

EXPORT FILE *xfile_open_from_fptr(FILE *fp, char const *mode);

#endif
