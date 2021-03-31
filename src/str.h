#ifndef STR_H
#define STR_H

#include "dcp/dcp.h"
#include "stream.h"

struct dcp_string
{
    struct stream stream;
    size_t        size;
};

static inline char* string_data(struct dcp_string* string) { return stream_data(&string->stream); }

static inline void string_deinit(struct dcp_string const* string) { stream_deinit(&string->stream); }

static inline void string_init(struct dcp_string* string)
{
    stream_init(&string->stream, 6);
    string->size = 0;
}

static inline void string_reuse(struct dcp_string* string) { string->size = 0; }

static inline void string_set(struct dcp_string* string, char c)
{
    size_t i = string->size;
    if (i >= stream_capacity(&string->stream))
        stream_grow(&string->stream);
    stream_data(&string->stream)[i] = c;
    ++string->size;
}

static inline void string_setend(struct dcp_string* string)
{
    string_set(string, '\0');
    --string->size;
}

static inline void string_rollback(struct dcp_string* string)
{
    if (string->size > 0)
        --string->size;
}

#endif
