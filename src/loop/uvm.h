#ifndef CORE_UVM_H
#define CORE_UVM_H

// ACK: https://github.com/litesync/libuv_message_framing

#include "uv.h"
#include <stdint.h>

struct uvm;
struct uvm_req;

typedef void (*uvm_free_cb)(uv_handle_t *handle, void *ptr);
typedef void (*uvm_read_cb)(struct uvm *, void *msg, int size);

int uvm_init(uv_loop_t *loop, struct uvm *, int stream_type);
int uvm_send(struct uvm_req *, struct uvm *, void *msg, int size, uv_write_cb);
int uvm_read_start(struct uvm *, uv_alloc_cb, uvm_read_cb, uvm_free_cb);

struct uvm
{
    union
    {
        uv_tcp_t tcp;
        uv_pipe_t pipe;
        void *data;
    };
    char *buf;
    int alloc_size;
    int filled;
    uv_alloc_cb alloc_cb;
    uvm_free_cb free_cb;
    uvm_read_cb read_cb;
};

struct uvm_req
{
    union
    {
        uv_write_t req;
        void *data;
    };
    uv_buf_t buf[2];
    int32_t msg_size;
};

#endif
