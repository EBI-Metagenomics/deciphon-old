#include "loop/writer_req_pool.h"
#include "core/logy.h"
#include "loop/writer_req.h"
#include <assert.h>
#include <stdbool.h>

static struct writer_req request = {NULL, {.data = &request}, 0, NULL};
static bool in_use = false;

struct writer_req *writer_req_pool_pop(struct writer *writer)
{
    if (in_use) return writer_req_new(writer);
    in_use = true;
    return &request;
}

void writer_req_pool_put(struct writer_req const *req)
{
    if (&request == req)
    {
        in_use = false;
        return;
    }
    writer_req_del(req);
}

void writer_req_pool_cleanup(void)
{
    assert(!in_use);
    writer_req_cleanup(&request);
}
