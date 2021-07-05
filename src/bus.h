#ifndef BUS_H
#define BUS_H

#include "support.h"
#include <ck_pr.h>
#include <ck_ring.h>

struct dcp_profile;

#define BUS_BUFFSIZE 256

struct bus
{
    int open_input;
    ck_ring_t ring CK_CC_CACHELINE;
    ck_ring_buffer_t buffer[BUS_BUFFSIZE];
};

static inline void bus_close_input(struct bus *bus)
{
    ck_pr_store_int(&bus->open_input, 0);
}
static inline bool bus_end(struct bus const *bus);
static inline void bus_init(struct bus *bus);
static inline void bus_open_input(struct bus *bus)
{
    ck_pr_store_int(&bus->open_input, 1);
}
static inline void *bus_recv(struct bus *bus);
static inline bool bus_send(struct bus *bus, void const *ptr);

static inline bool bus_end(struct bus const *bus)
{
    return !ck_pr_load_int(&bus->open_input) && ck_ring_size(&bus->ring) == 0;
}

static inline void bus_init(struct bus *bus)
{
    ck_pr_store_int(&bus->open_input, 0);
    ck_ring_init(&bus->ring, ARRAY_SIZE(bus->buffer));
    memset(bus->buffer, 0, ARRAY_SIZE(bus->buffer) * sizeof(*bus->buffer));
}

static inline void *bus_recv(struct bus *bus)
{
    void *ptr = NULL;
    return ck_ring_dequeue_spmc(&bus->ring, bus->buffer, &ptr) ? ptr : NULL;
}

static inline bool bus_send(struct bus *bus, void const *ptr)
{
    return ck_ring_enqueue_spmc(&bus->ring, bus->buffer, ptr);
}

#endif
