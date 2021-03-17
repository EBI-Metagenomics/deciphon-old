#ifndef STR_H
#define STR_H

#include "dcp/dcp.h"
#include "stream.h"

struct dcp_string
{
    struct stream stream;
};

static inline char* string_data(struct dcp_string* string) { return stream_data(&string->stream); }

static inline void string_deinit(struct dcp_string const* string) { stream_deinit(&string->stream); }

static inline void string_grow(struct dcp_string* string, size_t size)
{
    while (size > (stream_capacity(&string->stream) - stream_size(&string->stream)))
        stream_grow(&string->stream);
}

static inline void string_init(struct dcp_string* string) { stream_init(&string->stream, 6); }

#endif
