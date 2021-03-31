#ifndef STREAM_H
#define STREAM_H

#include <stddef.h>
#include <stdlib.h>

struct stream
{
    size_t capacity;
    char*  data;
};

static inline size_t stream_capacity(struct stream const* stream) { return stream->capacity; }

static inline char* stream_data(struct stream const* stream) { return stream->data; }

static inline void stream_deinit(struct stream const* stream) { free(stream->data); }

static inline void stream_grow(struct stream* stream)
{
    stream->capacity *= 2;
    stream->data = realloc(stream->data, sizeof(char) * stream->capacity);
}

static inline void stream_init(struct stream* stream, unsigned power_size)
{
    size_t capacity = 1 << power_size;
    stream->data = malloc(sizeof(char) * capacity);
    stream->capacity = capacity;
}

#endif
