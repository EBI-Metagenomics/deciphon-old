#include "loop/loopio.h"
#include "core/logging.h"

static void input_onclose(void *arg);
static void input_onopen(bool ok, void *arg);
static void output_onclose(void *arg);
static void output_onopen(bool ok, void *arg);
static void reader_onclose(void *arg);
static void reader_oneof(void *arg);
static void reader_onerror(void *arg);
static void reader_onread(char *line, void *arg);
static void writer_onerror(void *arg);
static void writer_onclose(void *arg);

void loopio_init(struct loopio *l, struct uv_loop_s *loop,
                 loopio_onread_fn_t *onread_cb, loopio_oneof_fn_t *oneof_cb,
                 loopio_onerror_fn_t onerror_cb, void *arg)
{
    l->loop = loop;
    fio_init(&l->input, loop, &input_onopen, &input_onclose, l);
    fio_init(&l->output, loop, &output_onopen, &output_onclose, l);
    reader_init(&l->reader, loop, &reader_oneof, &reader_onerror,
                &reader_onread, &reader_onclose, l);
    writer_init(&l->writer, loop, &writer_onerror, &writer_onclose, l);
    l->onread_cb = onread_cb;
    l->oneof_cb = oneof_cb;
    l->onerror_cb = onerror_cb;
    l->arg = arg;
    l->reader_noclose = true;
    l->writer_noclose = true;
}

void loopio_open(struct loopio *l, char const *input, char const *output)
{
    fio_open(&l->input, input, UV_FS_O_RDONLY, 0);
    l->reader_noclose = false;
    fio_open(&l->output, output, UV_FS_O_WRONLY, UV_FS_O_CREAT);
    l->writer_noclose = false;
}

void loopio_put(struct loopio *l, char const *msg)
{
    writer_put(&l->writer, msg);
}

void loopio_terminate(struct loopio *l)
{
    if (!l->reader_noclose) reader_close(&l->reader);
    if (!l->writer_noclose) writer_close(&l->writer);
}

static void input_onclose(void *arg) { (void)arg; }

static void input_onopen(bool ok, void *arg)
{
    struct loopio *l = arg;
    if (!ok)
    {
        eio("input open error");
        (*l->onerror_cb)(l->arg);
        return;
    }
    reader_fopen(&l->reader, fio_fd(&l->input));
}

static void output_onclose(void *arg) { (void)arg; }

static void output_onopen(bool ok, void *arg)
{
    struct loopio *l = arg;
    if (!ok)
    {
        eio("output open error");
        (*l->onerror_cb)(l->arg);
        return;
    }
    writer_fopen(&l->writer, fio_fd(&l->output));
}

static void reader_onclose(void *arg)
{
    struct loopio *l = arg;
    fio_close(&l->input);
}

static void reader_oneof(void *arg)
{
    struct loopio *l = arg;
    (*l->oneof_cb)(l->arg);
}

static void reader_onerror(void *arg)
{
    struct loopio *l = arg;
    (*l->onerror_cb)(l->arg);
}

static void reader_onread(char *line, void *arg)
{
    struct loopio *l = arg;
    (*l->onread_cb)(line, l->arg);
}

static void writer_onerror(void *arg)
{
    struct loopio *l = arg;
    (*l->onerror_cb)(l->arg);
}

static void writer_onclose(void *arg)
{
    struct loopio *l = arg;
    fio_close(&l->output);
}
