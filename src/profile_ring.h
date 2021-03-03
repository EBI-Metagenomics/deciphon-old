#ifndef PROFILE_RING_H
#define PROFILE_RING_H

#include "ck_ring.h"

struct dcp_profile;

#define PROFILE_RING_BUFFSIZE 256

struct profile_ring
{
    ck_ring_t ring   CK_CC_CACHELINE;
    ck_ring_buffer_t buffer[PROFILE_RING_BUFFSIZE];
};

struct profile_ring       profile_ring_init(void);
struct dcp_profile const* profile_ring_pop(struct profile_ring* ring);
void                      profile_ring_push(struct profile_ring* ring, struct dcp_profile const* profile);

#endif
