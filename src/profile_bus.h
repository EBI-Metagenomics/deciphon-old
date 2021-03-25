#ifndef PROFILE_BUS_H
#define PROFILE_BUS_H

#include "util.h"
#include <ck_pr.h>
#include <ck_ring.h>

struct dcp_profile;

#define PROFILE_BUS_BUFFSIZE 256

struct profile_bus
{
    int              closed;
    ck_ring_t ring   CK_CC_CACHELINE;
    ck_ring_buffer_t buffer[PROFILE_BUS_BUFFSIZE];
};

static inline void                      profile_bus_close(struct profile_bus* bus);
static inline bool                      profile_bus_closed(struct profile_bus const* bus);
static inline void                      profile_bus_deinit(struct profile_bus* bus);
static inline void                      profile_bus_init(struct profile_bus* bus);
static inline void                      profile_bus_open(struct profile_bus* bus);
static inline struct dcp_profile const* profile_bus_recv(struct profile_bus* bus);
static inline bool                      profile_bus_send(struct profile_bus* bus, struct dcp_profile const* profile);

static inline void profile_bus_close(struct profile_bus* bus) { ck_pr_store_int(&bus->closed, 1); }

static inline bool profile_bus_closed(struct profile_bus const* bus) { return ck_pr_load_int(&bus->closed); }

static inline void profile_bus_deinit(struct profile_bus* bus) { ck_pr_store_int(&bus->closed, 0); }

static inline void profile_bus_init(struct profile_bus* bus)
{
    ck_pr_store_int(&bus->closed, 1);
    ck_ring_init(&bus->ring, ARRAY_SIZE(bus->buffer));
    memset(bus->buffer, 0, ARRAY_SIZE(bus->buffer) * sizeof(*bus->buffer));
}

static inline void profile_bus_open(struct profile_bus* bus) { ck_pr_store_int(&bus->closed, 0); }

static inline struct dcp_profile const* profile_bus_recv(struct profile_bus* ring)
{
    struct dcp_profile const* prof = NULL;
    return ck_ring_dequeue_spmc(&ring->ring, ring->buffer, &prof) ? prof : NULL;
}

static inline bool profile_bus_send(struct profile_bus* ring, struct dcp_profile const* profile)
{
    return ck_ring_enqueue_spmc(&ring->ring, ring->buffer, profile);
}

#endif
