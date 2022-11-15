#ifndef LOOP_WREQ_REQ
#define LOOP_WREQ_REQ

struct writer;
struct wreq;

struct wreq *wreq_pop(struct writer *);
void wreq_put(struct wreq const *);
int wreq_size(struct wreq const *);
char *wreq_data(struct wreq *);
void wreq_setstr(struct wreq *, char const *string);
struct uv_write_s *wreq_uvreq(struct wreq *);
struct writer *wreq_writer(struct wreq *);

#endif
