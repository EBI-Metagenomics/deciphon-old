#ifndef DECY_SESSION_H
#define DECY_SESSION_H

struct uv_loop_s;

void decy_session_init(struct uv_loop_s *loop);
void decy_session_cleanup(void);

#endif
