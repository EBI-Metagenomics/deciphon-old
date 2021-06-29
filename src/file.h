#ifndef FILE_H
#define FILE_H

#include "cmp.h"

#define START ((imm_state_id_t)(0U << 11))
#define FILE_READ (1U << 0)
#define FILE_WRIT (1U << 1)

struct file
{
    FILE *fd;
    uint8_t mode;
    cmp_ctx_t ctx;
};

int file_close(struct file *f);

int file_open(struct file *f, const char *restrict filepath,
              const char *restrict mode);

int file_rewind(struct file *f);

#endif
