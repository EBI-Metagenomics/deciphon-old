#ifndef DCP_CMP_H
#define DCP_CMP_H

/* Wrapper around [CMP library](https://github.com/camgunz/cmp). */

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

struct dcp_cmp_ctx;

typedef bool (*dcp_cmp_reader)(struct dcp_cmp_ctx *ctx, void *data,
                               size_t limit);
typedef bool (*dcp_cmp_skipper)(struct dcp_cmp_ctx *ctx, size_t count);
typedef size_t (*dcp_cmp_writer)(struct dcp_cmp_ctx *ctx, const void *data,
                                 size_t count);

struct dcp_cmp_ctx
{
    uint8_t error;
    void *buf;
    dcp_cmp_reader read;
    dcp_cmp_skipper skip;
    dcp_cmp_writer write;
};

void dcp_cmp_init(struct dcp_cmp_ctx *cmp, FILE *file);

#endif
