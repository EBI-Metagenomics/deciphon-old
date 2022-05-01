#include "deciphon/ljson.h"

static void put_char(struct ljson_ctx *ctx, char c)
{
    if ((unsigned)(ctx->cur - ctx->buf) >= ctx->size)
    {
        ctx->error = 1;
    }
    else
    {
        *ctx->cur++ = c;
    }
}

static void put_raw(struct ljson_ctx *ctx, char const *str)
{
    while (*str != '\0')
        put_char(ctx, *str++);
}

static void put_str(struct ljson_ctx *ctx, char const *str)
{
    put_char(ctx, '\"');
    put_raw(ctx, str);
    put_char(ctx, '\"');
}

// swap the values in the two given variables
// XXX: fails when a and b refer to same memory location
#define XOR_SWAP(a, b)                                                         \
    do                                                                         \
    {                                                                          \
        a ^= b;                                                                \
        b ^= a;                                                                \
        a ^= b;                                                                \
    } while (0)

void ljson_open(struct ljson_ctx *ctx, unsigned size, char *str)
{
    for (unsigned i = 0; i < size; ++i)
        str[i] = '\0';
    ctx->size = size;
    ctx->buf = str;
    ctx->cur = str;
    ctx->elem = 0;
    ctx->error = 0;
    ctx->tmpbuf[0] = '\0';
    put_char(ctx, '{');
}

void ljson_close(struct ljson_ctx *ctx)
{
    if (!ctx->error)
    {
        put_char(ctx, '}');
        put_char(ctx, '\0');
    }
}

int ljson_error(struct ljson_ctx *ctx) { return ctx->error; }

static int write_object(struct ljson_ctx *ctx, char *key)
{
    if (!ctx->error)
    {
        if (ctx->elem++ > 0) put_char(ctx, ',');
        put_str(ctx, key);
        put_char(ctx, ':');
    }
    return ctx->error;
}

static void write_raw(struct ljson_ctx *ctx, char *key, char *rawtext)
{
    if (!write_object(ctx, key)) put_raw(ctx, rawtext);
}

void ljson_str(struct ljson_ctx *ctx, char *key, char const *str)
{
    if (!write_object(ctx, key)) put_str(ctx, str);
}

static int itoa_rev(char *buf, long i)
{
    char *dst = buf;
    if (i == 0)
    {
        *dst++ = '0';
    }
    int const neg = (i < 0) ? -1 : 1;
    while (i)
    {
        *dst++ = (char)('0' + (neg * (i % 10)));
        i /= 10;
    }
    if (neg == -1)
    {
        *dst++ = '-';
    }
    return (int)(dst - buf);
}

static void reverse(char *buf, int size)
{
    char *end = buf + size - 1;

    while (buf < end)
    {
        XOR_SWAP(*buf, *end);
        buf++;
        end--;
    }
}

void ljson_int(struct ljson_ctx *ctx, char *key, long value)
{
    int const size = itoa_rev(ctx->tmpbuf, value);
    reverse(ctx->tmpbuf, size);
    write_raw(ctx, key, ctx->tmpbuf);
}

void ljson_bool(struct ljson_ctx *ctx, char *key, int value)
{
    write_raw(ctx, key, value ? "true" : "false");
}

void ljson_null(struct ljson_ctx *ctx, char *key)
{
    write_raw(ctx, key, "null");
}
