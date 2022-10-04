#include "writer_req.h"
#include "xmem.h"
#include <stdlib.h>
#include <string.h>

struct writer_req *writer_req_new(struct writer *writer)
{
    struct writer_req *req = xmalloc(sizeof(*req));
    req->writer = writer;
    req->uvreq.data = req;
    req->size = 0;
    req->data = NULL;
    return req;
}

void writer_req_set_string(struct writer_req *req, char const *string)
{
    unsigned size = (unsigned)(strlen(string) + 1);
    req->data = xrealloc(req->data, size);
    req->size = size;
    memcpy(req->data, string, req->size);
}

void writer_req_cleanup(struct writer_req *req)
{
    free(req->data);
    req->data = NULL;
}

void writer_req_del(struct writer_req const *req)
{
    writer_req_cleanup((struct writer_req *)req);
    free((void *)req);
}
