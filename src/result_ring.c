#include "result_ring.h"
#include "util.h"

struct result_ring result_ring_init(void)
{
    struct result_ring ring;
    ck_ring_init(&ring.ring, ARRAY_SIZE(ring.buffer));
    memset(ring.buffer, 0, ARRAY_SIZE(ring.buffer) * sizeof(*ring.buffer));
    return ring;
}

struct dcp_result const* result_ring_pop(struct result_ring* ring)
{
    struct dcp_result const* result = NULL;
    bool                     ok = ck_ring_dequeue_mpmc(&ring->ring, ring->buffer, &result);
    if (ok)
        return result;
    return NULL;
}

void result_ring_push(struct result_ring* ring, struct dcp_result const* result)
{
    while (!ck_ring_enqueue_spsc(&ring->ring, ring->buffer, result))
        ck_pr_stall();
}
