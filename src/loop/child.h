#ifndef LOOP_CHILD_H
#define LOOP_CHILD_H

#include <stdbool.h>

struct child;

struct child *child_new(void);
void child_enable_stdin(struct child *, void (*error)());
void child_enable_stdout(struct child *, void (*read)(char *), void (*eof)(),
                         void (*error)());
void child_enable_stderr(struct child *);
void child_set_on_exit(struct child *, void (*on_exit)(int, void *));
void child_set_callb_arg(struct child *, void *);
void child_set_auto_delete(struct child *, bool);
bool child_spawn(struct child *, char const *args[]);
void child_send(struct child *, char const *string);
void child_kill(struct child *);
bool child_closed(struct child const *);
void child_close(struct child *);
void child_del(struct child *);

#endif
