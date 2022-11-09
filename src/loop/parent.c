#include "loop/parent.h"
#include <unistd.h>

void parent_init(struct parent *parent, on_read2_fn_t *on_read,
                 on_eof2_fn_t *on_eof, on_error2_fn_t *on_error)
{
    input_init(&parent->input, STDIN_FILENO);
    output_init(&parent->output, STDOUT_FILENO);
    parent->input.cb.on_read = on_read;
    parent->input.cb.on_eof = on_eof;
    parent->input.cb.on_error = on_error;
    parent->output.cb.on_error = on_error;
}

void parent_open(struct parent *parent)
{
    input_start(&parent->input);
    output_start(&parent->output);
}

void parent_send(struct parent *parent, char const *string)
{
    if (string) writer_put(&parent->output.writer, string);
}

void parent_close(struct parent *parent)
{
    input_stop(&parent->input);
    input_close(&parent->input);
    output_close(&parent->output);
}
