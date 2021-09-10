#ifndef DCP_CMP_H
#define DCP_CMP_H

/* Wrapper around [CMP library](https://github.com/camgunz/cmp). */

#include "dcp/export.h"
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

struct dcp_cmp;

typedef bool (*dcp_cmp_reader)(struct dcp_cmp *cmp, void *data, size_t limit);
typedef bool (*dcp_cmp_skipper)(struct dcp_cmp *cmp, size_t count);
typedef size_t (*dcp_cmp_writer)(struct dcp_cmp *cmp, const void *data,
                                 size_t count);

struct dcp_cmp
{
    uint8_t error;
    void *buf;
    dcp_cmp_reader read;
    dcp_cmp_skipper skip;
    dcp_cmp_writer write;
};

DCP_API void dcp_cmp_init(struct dcp_cmp *cmp, FILE *file);

static inline FILE *dcp_cmp_fd(struct dcp_cmp *cmp) { return (FILE *)cmp->buf; }

static inline int dcp_cmp_close(struct dcp_cmp *cmp)
{
    return fclose(dcp_cmp_fd(cmp));
}

static inline void dcp_cmp_rewind(struct dcp_cmp *cmp)
{
    rewind(dcp_cmp_fd(cmp));
}

#endif
