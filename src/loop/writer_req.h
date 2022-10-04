#ifndef LOOP_WRITER_REQ
#define LOOP_WRITER_REQ

#include "uv.h"

struct writer_req
{
    struct writer *writer;
    struct uv_write_s uvreq;
    unsigned size;
    char *data;
};

struct writer_req *writer_req_new(struct writer *writer);
void writer_req_set_string(struct writer_req *req, char const *string);
void writer_req_cleanup(struct writer_req *req);
void writer_req_del(struct writer_req const *req);

#endif
