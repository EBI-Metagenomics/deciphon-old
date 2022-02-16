#ifndef XCMP_H
#define XCMP_H

#include "cmp/cmp.h"

enum xcmp_mode
{
    XCMP_READ,
    XCMP_WRITE,
};

struct xcmp
{
    struct cmp_ctx_s cmp;
    FILE *fp;
    enum xcmp_mode mode;
    struct
    {
        off_t base;
        off_t pos;
        off_t fpos;
        struct buff *buff;
    } read;
};

bool xcmp_setup(struct xcmp *x, FILE *fp, enum xcmp_mode mode);
bool xcmp_load(struct xcmp *x, size_t size);
void xcmp_del(struct xcmp *x);

#endif
