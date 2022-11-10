#include "loop/parent.h"
#include "core/logy.h"
#include <assert.h>
#include <unistd.h>

static void on_exit_fwd(void *arg);

void parent_init(struct parent *parent, on_read2_fn_t *on_read,
                 on_eof2_fn_t *on_eof, on_error2_fn_t *on_error,
                 on_exit_fn_t *on_exit)
{
    input_init(&parent->input, STDIN_FILENO, &on_exit_fwd, parent);
    output_init(&parent->output, STDOUT_FILENO, &on_exit_fwd, parent);
    parent->input.on_read = on_read;
    parent->input.on_eof = on_eof;
    parent->input.on_error = on_error;
    parent->output.on_error = on_error;
    parent->on_exit = on_exit;
    parent->remain_handlers = 2;
}

void parent_start(struct parent *parent)
{
    input_start(&parent->input);
    output_start(&parent->output);
}

void parent_send(struct parent *parent, char const *string)
{
    if (string) writer_put(&parent->output.writer, string);
}

void parent_stop(struct parent *parent)
{
    input_stop(&parent->input);
    input_cleanup(&parent->input);
    output_cleanup(&parent->output);
}

void parent_cleanup(struct parent *parent)
{
    input_cleanup(&parent->input);
    output_cleanup(&parent->output);
}

bool parent_exitted(struct parent const *parent)
{
    return !parent->remain_handlers;
}

static void on_exit_fwd(void *arg)
{
    struct parent *parent = arg;
    if (!--parent->remain_handlers && parent->on_exit) (*parent->on_exit)();
}
