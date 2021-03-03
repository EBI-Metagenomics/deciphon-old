#ifndef RESULT_RING_H
#define RESULT_RING_H

#include "ck_ring.h"

struct dcp_result;

#define RESULT_RING_BUFFSIZE 1024

struct result_ring
{
    ck_ring_t ring   CK_CC_CACHELINE;
    ck_ring_buffer_t buffer[RESULT_RING_BUFFSIZE];
};

struct result_ring       result_ring_init(void);
struct dcp_result const* result_ring_pop(struct result_ring* ring);
void                     result_ring_push(struct result_ring* ring, struct dcp_result const* result);

#endif
