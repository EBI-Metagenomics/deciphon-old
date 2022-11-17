#ifndef LOOP_HALT_SIGNAL
#define LOOP_HALT_SIGNAL

struct uv_loop_s;
typedef void on_signal_fn_t(void);

void halt_signal_open(struct uv_loop_s *, on_signal_fn_t *);
void halt_signal_close(void);

#endif
