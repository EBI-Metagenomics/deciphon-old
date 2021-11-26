#ifndef DCP_CMP_H
#define DCP_CMP_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

struct cmp_ctx_s;

typedef bool (*cmp_reader)(struct cmp_ctx_s *ctx, void *data, size_t limit);
typedef bool (*cmp_skipper)(struct cmp_ctx_s *ctx, size_t count);
typedef size_t (*cmp_writer)(struct cmp_ctx_s *ctx, const void *data,
                             size_t count);

typedef struct cmp_ctx_s
{
    uint8_t error;
    void *buf;
    cmp_reader read;
    cmp_skipper skip;
    cmp_writer write;
} cmp_ctx_t;

#endif
