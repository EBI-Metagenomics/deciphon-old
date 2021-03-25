#ifndef BUS_H
#define BUS_H

#include "util.h"
#include <ck_pr.h>
#include <ck_ring.h>

struct dcp_profile;

#define BUS_BUFFSIZE 256

struct bus
{
    int              closed;
    ck_ring_t ring   CK_CC_CACHELINE;
    ck_ring_buffer_t buffer[BUS_BUFFSIZE];
};

static inline void  bus_close(struct bus* bus);
static inline bool  bus_closed(struct bus const* bus);
static inline void  bus_deinit(struct bus* bus);
static inline void  bus_init(struct bus* bus);
static inline void  bus_open(struct bus* bus);
static inline void* bus_recv(struct bus* bus);
static inline bool  bus_send(struct bus* bus, void const* ptr);

static inline void bus_close(struct bus* bus) { ck_pr_store_int(&bus->closed, 1); }

static inline bool bus_closed(struct bus const* bus) { return ck_pr_load_int(&bus->closed); }

static inline void bus_deinit(struct bus* bus) { ck_pr_store_int(&bus->closed, 0); }

static inline void bus_init(struct bus* bus)
{
    ck_pr_store_int(&bus->closed, 1);
    ck_ring_init(&bus->ring, ARRAY_SIZE(bus->buffer));
    memset(bus->buffer, 0, ARRAY_SIZE(bus->buffer) * sizeof(*bus->buffer));
}

static inline void bus_open(struct bus* bus) { ck_pr_store_int(&bus->closed, 0); }

static inline void* bus_recv(struct bus* bus)
{
    void* ptr = NULL;
    return ck_ring_dequeue_spmc(&bus->ring, bus->buffer, &ptr) ? ptr : NULL;
}

static inline bool bus_send(struct bus* bus, void const* ptr)
{
    return ck_ring_enqueue_spmc(&bus->ring, bus->buffer, ptr);
}

#endif
