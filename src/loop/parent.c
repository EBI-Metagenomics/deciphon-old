#include "loop/parent.h"
#include "core/logy.h"
#include <assert.h>
#include <unistd.h>

static void on_exit_fwd(void *arg);

void parent_init(struct parent *parent, on_read2_fn_t *on_read,
                 on_eof2_fn_t *on_eof, on_error2_fn_t *on_error,
                 on_exit_fn_t *on_exit)
{
    input_init(&parent->input, STDIN_FILENO, on_read, on_eof, on_error,
               &on_exit_fwd, parent);
    output_init(&parent->output, STDOUT_FILENO, on_error, &on_exit_fwd, parent);
    parent->on_exit = on_exit;
    parent->remain_handlers = 2;
}

void parent_open(struct parent *parent) { input_start(&parent->input); }

void parent_send(struct parent *parent, char const *string)
{
    if (string) writer_put(&parent->output.writer, string);
}

void parent_close(struct parent *parent)
{
    input_close(&parent->input);
    output_close(&parent->output);
}

void parent_cleanup(struct parent *parent)
{
    input_cleanup(&parent->input);
    output_close(&parent->output);
}

bool parent_offline(struct parent const *parent)
{
    return !parent->remain_handlers;
}

static void on_exit_fwd(void *arg)
{
    struct parent *parent = arg;
    if (!--parent->remain_handlers && parent->on_exit) (*parent->on_exit)();
}
