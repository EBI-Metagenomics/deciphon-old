#include "uvm.h"
#include "core/pp.h"
#include "zc.h"
#include <assert.h>
#include <stdlib.h>

#define WIDTH (int)sizeof_field(struct uvm_req, msg_size)

int uvm_init(uv_loop_t *loop, struct uvm *uvm, int stream_type)
{
    assert(stream_type == UV_TCP || stream_type == UV_NAMED_PIPE);

    int rc = 0;

    switch (stream_type)
    {
    case UV_TCP:
        rc = uv_tcp_init(loop, (uv_tcp_t *)uvm);
        break;
    case UV_NAMED_PIPE:
        rc = uv_pipe_init(loop, (uv_pipe_t *)uvm, 0);
        break;
    default:
        return UV_EINVAL;
    }

    if (rc) return rc;

    uvm->data = NULL;
    uvm->buf = NULL;
    uvm->alloc_size = 0;
    uvm->filled = 0;
    uvm->alloc_cb = NULL;
    uvm->free_cb = NULL;
    uvm->read_cb = NULL;

    return 0;
}

int uvm_send(struct uvm_req *req, struct uvm *uvm, void *msg, int size,
             uv_write_cb write_cb)
{
    uv_stream_t *stream = (uv_stream_t *)uvm;

    assert(!(!req || !stream || !msg || size <= 0));
    if (!req || !stream || !msg || size <= 0) return UV_EINVAL;

    req->msg_size = zc_htonl((uint32_t)size);
    req->buf[0].base = (char *)&req->msg_size;
    req->buf[0].len = WIDTH;
    req->buf[1] = uv_buf_init(msg, size);

    return uv_write((uv_write_t *)req, stream, &req->buf[0], 2, write_cb);
}

static void msg_alloc(uv_handle_t *handle, size_t suggested_size,
                      uv_buf_t *stream_buf);
static void msg_read(uv_stream_t *stream, ssize_t nread, const uv_buf_t *buf);

int uvm_read_start(struct uvm *uvm, uv_alloc_cb alloc_cb, uvm_read_cb read_cb,
                   uvm_free_cb free_cb)
{
    uvm->read_cb = read_cb;
    uvm->alloc_cb = alloc_cb;
    uvm->free_cb = free_cb;

    return uv_read_start((uv_stream_t *)uvm, msg_alloc, msg_read);
}

static void msg_free(struct uvm *uvm)
{
    if (uvm->free_cb) uvm->free_cb((uv_handle_t *)uvm, uvm->buf);
    uvm->buf = 0;
    uvm->alloc_size = 0;
}

static int msg_realloc(uv_handle_t *handle, size_t suggested_size)
{
    struct uvm *uvm = (struct uvm *)handle;
    uv_buf_t buf = {0};
    uvm->alloc_cb(handle, suggested_size, &buf);
    if (buf.base == 0 || buf.len < suggested_size)
        return 0; //! if buf.len < suggested_size and buf.base is valid it will
                  //! be lost here (the allocated memory)
    memcpy(buf.base, uvm->buf, uvm->filled);
    if (uvm->free_cb) uvm->free_cb(handle, uvm->buf);
    uvm->buf = buf.base;
    uvm->alloc_size = buf.len;
    return 1;
}

static void msg_alloc(uv_handle_t *handle, size_t suggested_size,
                      uv_buf_t *stream_buf)
{
    struct uvm *uvm = (struct uvm *)handle;

    if (!uvm) return;

    if (uvm->buf == 0)
    {
        uv_buf_t buf = {0};
        uvm->alloc_cb(handle, suggested_size, &buf);
        uvm->buf = buf.base;
        if (uvm->buf == 0) return;
        uvm->alloc_size = buf.len;
    }

    if (uvm->filled >= WIDTH)
    {
        int msg_size = (int)zc_ntohl(*(uint32_t *)uvm->buf);
        int entire_msg_size = msg_size + WIDTH;
        if (uvm->alloc_size < entire_msg_size)
        {
            /* here the suggested size is exactly what it's needed to read the
             * entire message */
            if (!msg_realloc(handle, entire_msg_size))
            {
                stream_buf->base = 0;
                return;
            }
        }
        stream_buf->len = entire_msg_size - uvm->filled;
    }
    else
    {
        if (uvm->alloc_size < WIDTH)
        {
            /* There is no enough space for the message length. Allocate the
             * default size */
            if (!msg_realloc(handle, 64 * 1024))
            {
                stream_buf->base = 0;
                return;
            }
        }
        stream_buf->len = uvm->alloc_size - uvm->filled;
    }

    stream_buf->base = uvm->buf + uvm->filled;
}

static void msg_read(uv_stream_t *stream, ssize_t nread, const uv_buf_t *buf)
{
    (void)buf;
    struct uvm *uvm = (struct uvm *)stream;

    if (uvm == 0) return;

    if (nread == 0)
    {
        /* Nothing read */
        //! does it should release the ->buf here?
        // msg_free(uvmsg);
        return;
    }

    if (nread < 0)
    {
        /* Error */
        msg_free(uvm);
        uvm->read_cb((struct uvm *)stream, NULL, nread);
        return;
    }

    uvm->filled += nread;

    char *ptr = uvm->buf;

    while (uvm->filled >= WIDTH)
    {
        int msg_size = (int)zc_ntohl(*(uint32_t *)ptr);
        int entire_msg = msg_size + WIDTH;
        if (uvm->filled >= entire_msg)
        {
            uvm->read_cb((struct uvm *)stream, ptr + WIDTH, msg_size);
            if (uvm->filled > entire_msg) ptr += entire_msg;
            uvm->filled -= entire_msg;
        }
        else
        {
            break;
        }
    }

    if (ptr > uvm->buf && uvm->filled > 0)
    {
        memmove(uvm->buf, ptr, uvm->filled);
    }
    else if (uvm->filled == 0)
    {
        msg_free(uvm);
    }
}
