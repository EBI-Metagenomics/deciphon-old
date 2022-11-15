#include "loop/wreq.h"
#include "core/logy.h"
#include "uv.h"
#include <string.h>

enum
{
    MAX_REQUESTS = 16,
    DATA_SIZE = 2048,
};

struct wreq
{
    struct writer *writer;
    struct uv_write_s uvreq;
    int id;
    int size;
    char data[DATA_SIZE];
};

static int in_use[MAX_REQUESTS] = {0};
static struct wreq requests[MAX_REQUESTS] = {0};

struct wreq *wreq_pop(struct writer *writer)
{
    struct wreq *req = NULL;
    for (int i = 0; i < MAX_REQUESTS; ++i)
    {
        if (!in_use[i])
        {
            in_use[i] = 1;
            req = requests + i;
            req->id = i;
            break;
        }
    }
    if (!req) fatal("writer requests reached limit");
    req->writer = writer;
    req->uvreq.data = req;
    req->size = 0;
    req->data[0] = '\0';
    return req;
}

void wreq_put(struct wreq const *req) { in_use[req->id] = 0; }

int wreq_size(struct wreq const *req) { return req->size; }

char *wreq_data(struct wreq *req) { return req->data; }

void wreq_setstr(struct wreq *req, char const *string)
{
    if (strlen(string) + 1 >= DATA_SIZE) fatal("request string size overflow");
    int size = (int)(strlen(string) + 1);
    req->size = size;
    memcpy(req->data, string, req->size);
}

struct uv_write_s *wreq_uvreq(struct wreq *req) { return &req->uvreq; }

struct writer *wreq_writer(struct wreq *req) { return req->writer; }
