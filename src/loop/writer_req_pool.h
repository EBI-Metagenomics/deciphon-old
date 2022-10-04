#ifndef PIPE_WRITER_REQ_POOL
#define PIPE_WRITER_REQ_POOL

struct writer;
struct writer_req;

struct writer_req *writer_req_pool_pop(struct writer *writer);
void writer_req_pool_put(struct writer_req const *);
void writer_req_pool_cleanup(void);

#endif
