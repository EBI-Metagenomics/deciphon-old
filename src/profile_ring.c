#include "profile_ring.h"
#include "util.h"

struct profile_ring profile_ring_init(void)
{
    struct profile_ring ring;
    ck_ring_init(&ring.ring, ARRAY_SIZE(ring.buffer));
    memset(ring.buffer, 0, ARRAY_SIZE(ring.buffer) * sizeof(*ring.buffer));
    return ring;
}

struct dcp_profile const* profile_ring_pop(struct profile_ring* ring)
{
    struct dcp_profile const* prof = NULL;
    bool                      ok = ck_ring_dequeue_spmc(&ring->ring, ring->buffer, &prof);
    if (ok)
        return prof;
    return NULL;
}

void profile_ring_push(struct profile_ring* ring, struct dcp_profile const* profile)
{
    while (!ck_ring_enqueue_spmc(&ring->ring, ring->buffer, profile))
        ck_pr_stall();
}
