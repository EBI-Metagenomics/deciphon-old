#ifndef STREAM_H
#define STREAM_H

#include <assert.h>
#include <stdio.h>
#include <string.h>

#define STREAM_SIZE 128

struct stream
{
    unsigned pos;
    unsigned capacity;
    char data[STREAM_SIZE];
};

#define STREAM_INIT(capacity)                                                  \
    (struct stream)                                                            \
    {                                                                          \
        0, capacity, { '\0' }                                                  \
    }

static inline void stream_reset(struct stream *stream)
{
    stream->pos = 0;
    stream->data[0] = '\0';
}

static inline void stream_print(struct stream *stream, char const *src,
                                unsigned size)
{
    memcpy(stream->data + stream->pos, src, size);
    stream->pos += size;
}

static inline void stream_flush(struct stream *stream, FILE *restrict fd)
{
    fprintf(fd, "%*s", stream->pos, stream->data);
    stream_reset(stream);
}

static inline unsigned stream_size(struct stream const *stream)
{
    assert(stream->pos <= stream->capacity);
    return stream->capacity - stream->pos;
}

#endif
